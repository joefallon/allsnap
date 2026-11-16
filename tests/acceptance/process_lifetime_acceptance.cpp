#include "../support/ProcessFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(ProcessLifetimeAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        // HL006: actual target termination must not strand controller shutdown.
        TEST_METHOD(ForcedTargetTerminationDoesNotStrandShutdown)
        {
            ProcessFixture fixture;
            ProcessChild & controller = fixture.Child();
            Assert::IsTrue(controller.Call(ProcessStartHook).success != FALSE);
            ProcessChild & target = fixture.Child();
            Assert::IsTrue(target.Call(ProcessRegister).success != FALSE);
            Assert::IsFalse(controller.Call(ProcessQuery).canStop != FALSE);
            target.Terminate();
            Assert::IsTrue(controller.Call(ProcessQuery).canStop != FALSE,
                L"A terminated target cannot keep controller shutdown busy forever.");
            Assert::IsTrue(controller.Call(ProcessStopHook).success != FALSE);
        }

        // HL007: actual process detach releases only the exiting owner.
        TEST_METHOD(NormalTargetExitReleasesItsRegistration)
        {
            ProcessFixture fixture;
            ProcessChild & controller = fixture.Child();
            Assert::IsTrue(controller.Call(ProcessStartHook).success != FALSE);
            ProcessChild & target = fixture.Child();
            Assert::IsTrue(target.Call(ProcessRegister).success != FALSE);
            target.ExitNormally();
            Assert::IsTrue(controller.Call(ProcessQuery).canStop != FALSE);
            Assert::IsTrue(controller.Call(ProcessStopHook).success != FALSE);
        }

        // HL007: unrelated process detach must preserve the registered target.
        TEST_METHOD(UnrelatedProcessExitPreservesLiveOwner)
        {
            ProcessFixture fixture;
            ProcessChild & controller = fixture.Child();
            Assert::IsTrue(controller.Call(ProcessStartHook).success != FALSE);
            ProcessChild & target = fixture.Child();
            Assert::IsTrue(target.Call(ProcessRegister).success != FALSE);
            fixture.Child().ExitNormally();
            Assert::IsFalse(controller.Call(ProcessQuery).canStop != FALSE);
            Assert::IsFalse(controller.Call(ProcessStopHook).success != FALSE);
            Assert::IsTrue(target.Call(ProcessFinishMove).success != FALSE);
            Assert::IsTrue(controller.Call(ProcessStopHook).success != FALSE);
        }

        // HL008: two genuine processes retain independent registrations.
        TEST_METHOD(TwoLiveOwnersReleaseIndependently)
        {
            ProcessFixture fixture;
            ProcessChild & controller = fixture.Child();
            Assert::IsTrue(controller.Call(ProcessStartHook).success != FALSE);
            ProcessChild & first = fixture.Child();
            ProcessChild & second = fixture.Child();
            Assert::IsTrue(first.Call(ProcessRegister).success != FALSE);
            Assert::IsTrue(second.Call(ProcessRegister).success != FALSE);
            Assert::IsFalse(controller.Call(ProcessStopHook).success != FALSE);
            first.ExitNormally();
            Assert::IsFalse(controller.Call(ProcessStopHook).success != FALSE);
            Assert::IsTrue(second.Call(ProcessFinishMove).success != FALSE);
            Assert::IsTrue(controller.Call(ProcessStopHook).success != FALSE);
        }

        // HL009: a reserved native installation blocks another process's shutdown.
        TEST_METHOD(ReservedNativeInstallationBlocksConcurrentShutdown)
        {
            ProcessFixture fixture;
            ProcessChild & controller = fixture.Child();
            Assert::IsTrue(controller.Call(ProcessStartHook).success != FALSE);
            ProcessChild & target = fixture.Child();
            Assert::IsTrue(target.Call(ProcessArmReservation).success != FALSE);
            target.Begin(ProcessRegister);
            target.WaitForReservation();
            ProcessResult stopped = controller.Call(ProcessStopHook);
            target.ReleaseReservation();
            ProcessResult installed = target.Receive();
            Assert::IsFalse(stopped.success != FALSE);
            Assert::AreEqual<DWORD>(ERROR_BUSY, stopped.error);
            Assert::IsTrue(installed.success != FALSE);
            Assert::IsTrue(target.Call(ProcessFinishMove).success != FALSE);
            Assert::IsTrue(controller.Call(ProcessStopHook).success != FALSE);
        }

        // HL006: death during reservation must be recoverable as well.
        TEST_METHOD(TerminatedReservedInstallationDoesNotStrandShutdown)
        {
            ProcessFixture fixture;
            ProcessChild & controller = fixture.Child();
            Assert::IsTrue(controller.Call(ProcessStartHook).success != FALSE);
            ProcessChild & target = fixture.Child();
            Assert::IsTrue(target.Call(ProcessArmReservation).success != FALSE);
            target.Begin(ProcessRegister);
            target.WaitForReservation();
            Assert::IsFalse(controller.Call(ProcessStopHook).success != FALSE);
            target.Terminate();
            Assert::IsTrue(controller.Call(ProcessQuery).canStop != FALSE,
                L"A reservation cannot survive termination of its owning process.");
            Assert::IsTrue(controller.Call(ProcessStopHook).success != FALSE);
        }

        // HL008: recovering a dead registration must preserve the survivor.
        TEST_METHOD(ForcedDeathPreservesAnotherLiveOwner)
        {
            ProcessFixture fixture;
            ProcessChild & controller = fixture.Child();
            Assert::IsTrue(controller.Call(ProcessStartHook).success != FALSE);
            ProcessChild & terminated = fixture.Child();
            ProcessChild & survivor = fixture.Child();
            Assert::IsTrue(terminated.Call(ProcessRegister).success != FALSE);
            Assert::IsTrue(survivor.Call(ProcessRegister).success != FALSE);
            terminated.Terminate();
            Assert::IsFalse(controller.Call(ProcessStopHook).success != FALSE,
                L"Reaping a dead target must preserve another process's native subclass.");
            Assert::IsTrue(survivor.Call(ProcessFinishMove).success != FALSE);
            Assert::IsTrue(controller.Call(ProcessStopHook).success != FALSE);
        }

        // HL011: controller restart must preserve another process's ownership.
        TEST_METHOD(RestartedControllerPreservesLiveTarget)
        {
            ProcessFixture fixture;
            ProcessChild & original = fixture.Child();
            Assert::IsTrue(original.Call(ProcessStartHook).success != FALSE);
            ProcessChild & target = fixture.Child();
            Assert::IsTrue(target.Call(ProcessRegister).success != FALSE);
            original.ExitNormally();
            ProcessChild & restarted = fixture.Child();
            Assert::IsTrue(restarted.Call(ProcessStartHook).success != FALSE);
            Assert::IsFalse(restarted.Call(ProcessStopHook).success != FALSE,
                L"Starting a new controller must not erase an already-live target registration.");
            Assert::IsTrue(target.Call(ProcessFinishMove).success != FALSE);
            Assert::IsTrue(restarted.Call(ProcessStopHook).success != FALSE);
        }

        // HL012: the stopping gate applies to an already-loaded target process.
        TEST_METHOD(CompletedShutdownClosesRegistrationGateInAnotherProcess)
        {
            ProcessFixture fixture;
            ProcessChild & controller = fixture.Child();
            Assert::IsTrue(controller.Call(ProcessStartHook).success != FALSE);
            ProcessChild & target = fixture.Child();
            Assert::IsTrue(controller.Call(ProcessStopHook).success != FALSE);
            ProcessResult registered = target.Call(ProcessRegister);
            Assert::IsFalse(registered.success != FALSE);
            Assert::AreEqual<DWORD>(ERROR_BUSY, registered.error);
            Assert::IsFalse(registered.localOwner != FALSE);
        }

        // HL010: an installed native callback retains its code module.
        TEST_METHOD(OrdinaryModuleReleasePreservesInstalledNativeCallback)
        {
            AssertCallbackSurvivesModuleRelease(ProcessReleaseModule);
        }

        // HL010: actual callback execution remains safe across reference release.
        TEST_METHOD(OrdinaryModuleReleasePreservesExecutingNativeCallback)
        {
            AssertCallbackSurvivesModuleRelease(ProcessReleaseDuringCallback);
        }

        // HL013: hook entry retains code even when no subclass is installed.
        TEST_METHOD(NegativeHookCallbackRetainsItsCodeModule)
        {
            ProcessFixture fixture;
            ProcessChild & controller = fixture.Child();
            Assert::IsTrue(controller.Call(ProcessStartHook).success != FALSE);
            ProcessChild & target = fixture.Child();
            Assert::IsTrue(target.Call(ProcessEnterHook).success != FALSE);
            ProcessResult released = target.Call(ProcessReleaseModule);
            Assert::IsTrue(released.moduleMapped != FALSE,
                L"Entering the hook callback must retain its module even without a subclass registration.");
            Assert::IsTrue(released.success != FALSE);
            Assert::IsFalse(released.localOwner != FALSE);
            Assert::IsTrue(controller.Call(ProcessStopHook).success != FALSE);
            target.ExitNormally();
        }

    private:
        static void AssertCallbackSurvivesModuleRelease(DWORD command)
        {
            ProcessFixture fixture;
            ProcessChild & controller = fixture.Child();
            Assert::IsTrue(controller.Call(ProcessStartHook).success != FALSE);
            ProcessChild & target = fixture.Child();
            Assert::IsTrue(target.Call(ProcessRegister).success != FALSE);
            ProcessResult released = target.Call(command);
            Assert::IsTrue(released.moduleMapped != FALSE,
                L"An installed or executing native callback must retain its code module.");
            Assert::IsTrue(released.success != FALSE);
            Assert::IsFalse(released.localOwner != FALSE,
                L"The actual cleanup callback must complete after the ordinary module reference is released.");
            Assert::IsTrue(controller.Call(ProcessStopHook).success != FALSE);
            target.ExitNormally();
        }
    };
}
