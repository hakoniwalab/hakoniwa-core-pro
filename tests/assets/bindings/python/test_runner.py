

class TestRunner:
    def __init__(self, pdu_manager):
        self.pdu_manager = pdu_manager
        self.tests = []
        self.failed = 0

    def add(self, test_func):
        self.tests.append(test_func)

    def run(self):
        for test in self.tests:
            try:
                test(self.pdu_manager)
                print(f"[PASS] {test.__name__}")
            except AssertionError as e:
                print(f"[FAIL] {test.__name__}: {e}")
                self.failed += 1

        total = len(self.tests)
        passed = total - self.failed
        print(f"\n✅ Passed: {passed} / {total}")
        if self.failed > 0:
            exit(1)
    