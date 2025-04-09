from test_runner import TestRunner

def test_example():
    assert 1 + 1 == 2
    assert "hako".upper() == "HAKO"

if __name__ == "__main__":
    runner = TestRunner()
    runner.add(test_example)
    # runner.add(他のテスト関数)
    runner.run()
