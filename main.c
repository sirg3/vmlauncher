// Copyright (c) 2014 Joe Ranieri
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//   1. The origin of this software must not be misrepresented; you must not
//   claim that you wrote the original software. If you use this software
//   in a product, an acknowledgment in the product documentation would be
//   appreciated but is not required.
//
//   2. Altered source versions must be plainly marked as such, and must not be
//   misrepresented as being the original software.
//
//   3. This notice may not be removed or altered from any source
//   distribution.
//
//--
// This is a simple tool meant to act as a launchd daemon that manages the
// lifetime of a single VMWare Fusion virtual machine:
// * On launch, it attempts to power on the virtual machine.
// * While running, it mornitors the virtual machine's state and will power it
//   on again if it shuts down.
// * Upon exit, it will suspend the virtual machine.
//
// In the event of some sort of error, the daemon will exit with a non-zero
// status code.

#include <asl.h>
#include <assert.h>
#include <dispatch/dispatch.h>
#include <stdio.h>
#include "vix.h"

static VixHandle gHostHandle = VIX_INVALID_HANDLE;
static VixHandle gVMHandle = VIX_INVALID_HANDLE;
static const char *gVMName;

/// The number of nanoseconds to wait before polling the virtual machine's
/// status.
#define VM_POLL_INTERVAL (NSEC_PER_SEC * 30)

/// Checks to see if the error indicates failure.
/// \param err The error code to check.
/// \param msg The name of the operation being performed, for logging purposes.
static bool DidFail(VixError err, const char *msg) {
  if (VIX_FAILED(err)) {
    const char *errorText = Vix_GetErrorText(err, NULL);
    asl_log(NULL, NULL, ASL_LEVEL_ERR, "%s: %s [%llu]", msg, errorText, err);
    return true;
  }

  return false;
}

/// Releases a handle and sets it to be invalid.
/// \param handle The ViX handle to release (must be non-NULL).
static void ReleaseHandle(VixHandle *handle) {
  assert(handle);
  Vix_ReleaseHandle(*handle);
  *handle = VIX_INVALID_HANDLE;
}

/// Cleans up all state and exits the process.
/// \param exitStatus The return code to pass to exit.
static NORETURN void CleanupVM(int exitStatus) {
  if (gVMHandle != VIX_INVALID_HANDLE) {
    ReleaseHandle(&gVMHandle);
  }

  if (gHostHandle != VIX_INVALID_HANDLE) {
    VixHost_Disconnect(gHostHandle);
    ReleaseHandle(&gHostHandle);
  }

  exit(exitStatus);
}

/// Opens the specified 'vmx' file but does not start it.
/// \param vmPath The path to the 'vmx' file (not the 'vmwarevm' wrapper).
static void ConnectToVM(const char *vmPath) {
  VixError err = VIX_OK;
  VixHandle job = VIX_INVALID_HANDLE;

  job = VixHost_Connect(VIX_API_VERSION, VIX_SERVICEPROVIDER_VMWARE_WORKSTATION,
                        NULL, 0, NULL, NULL, 0, VIX_INVALID_HANDLE, NULL, NULL);
  err = VixJob_Wait(job, VIX_PROPERTY_JOB_RESULT_HANDLE, &gHostHandle,
                    VIX_PROPERTY_NONE);
  if (DidFail(err, "VixHost_Connect")) {
    CleanupVM(EXIT_FAILURE);
  }
  ReleaseHandle(&job);

  job = VixVM_Open(gHostHandle, vmPath, NULL, NULL);
  err = VixJob_Wait(job, VIX_PROPERTY_JOB_RESULT_HANDLE, &gVMHandle,
                    VIX_PROPERTY_NONE);
  if (DidFail(err, "VixVM_Open")) {
    VixHost_Disconnect(gHostHandle);
    CleanupVM(EXIT_FAILURE);
  }
  ReleaseHandle(&job);
}

/// Starts up the virtual machine if it isn't already running.
static void PowerOn() {
  asl_log(NULL, NULL, ASL_LEVEL_ERR, "Powering on %s", gVMName);

  VixHandle job = VixVM_PowerOn(gVMHandle, VIX_VMPOWEROP_NORMAL,
                                VIX_INVALID_HANDLE, NULL, NULL);
  VixError err = VixJob_Wait(job, VIX_PROPERTY_NONE);
  if (DidFail(err, "VixVM_PowerOn")) {
    CleanupVM(EXIT_FAILURE);
  }
  ReleaseHandle(&job);

  asl_log(NULL, NULL, ASL_LEVEL_ERR, "Successfully powered on %s", gVMName);
}

/// Handles the SIGTERM sent by launchd when the job is being shut down.
static NORETURN void HandleSIGTERM(void *context) {
  (void)context; // unused
  
  asl_log(NULL, NULL, ASL_LEVEL_ERR, "Received sigterm, suspending %s",
         gVMName);
  
  // Suspend the virtual machine. If it's not running, we'll get an error
  // and that's okay.
  VixHandle jobHandle = VixVM_Suspend(gVMHandle, 0, NULL, NULL);
  VixError err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
  if (VIX_ERROR_CODE(err) != VIX_E_VM_NOT_RUNNING) {
    DidFail(err, "VixVM_Suspend");
  }
  ReleaseHandle(&jobHandle);

  asl_log(NULL, NULL, ASL_LEVEL_ERR, "Successfully suspended %s", gVMName);
  CleanupVM(EXIT_SUCCESS);
}

/// Checks to see if the virtual machine is still running. If not, it will be
/// restarted.
static void CheckVMState(void *context) {
  (void)context; // unused
  
  int powerState = 0;
  VixError err = Vix_GetProperties(gVMHandle, VIX_PROPERTY_VM_POWER_STATE,
                                   &powerState, VIX_PROPERTY_NONE);
  if (DidFail(err, "Vix_GetProperties")) {
    CleanupVM(EXIT_FAILURE);
  }

  if (powerState & VIX_POWERSTATE_POWERED_OFF) {
    asl_log(NULL, NULL, ASL_LEVEL_ERR, "Restarting VM, state=0x%x", powerState);
    PowerOn();
  }
}

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s vmx_path\n", getprogname());
    exit(EXIT_FAILURE);
  }
  gVMName = argv[1];

  // Disable SIGTERM's default handler, which is to terminate.
  signal(SIGTERM, SIG_IGN);

  ConnectToVM(argv[1]);
  PowerOn();

  // launchd will send us a SIGTERM when we need to clean up and exit.
  dispatch_source_t source = dispatch_source_create(
      DISPATCH_SOURCE_TYPE_SIGNAL, SIGTERM, 0, dispatch_get_main_queue());
  dispatch_source_set_event_handler_f(source, &HandleSIGTERM);
  dispatch_resume(source);

  // VMWare has no API to get notified when the state of a VM changes, despite
  // having an obnoxious background thread that polls with a one second timeout.
  // So we'll have our own obnoxious polling timer.
  source = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0,
                                  dispatch_get_main_queue());
  dispatch_source_set_event_handler_f(source, &CheckVMState);
  dispatch_source_set_timer(source,
                            dispatch_time(DISPATCH_TIME_NOW, VM_POLL_INTERVAL),
                            VM_POLL_INTERVAL, 0);
  dispatch_resume(source);

  dispatch_main();
}
