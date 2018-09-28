import json
import os
import subprocess
import unittest


def dumps(obj):
    return json.dumps(obj).encode()


def zdumps(obj):
    return b'{}' + dumps(obj)


loads = json.loads


class TestExecutableMonostate(object):
    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, '_instance'):
            cls._instance = super().__new__(cls, *args, **kwargs)

        return cls._instance

    def __init__(self):
        if getattr(self, 'initialized', None):
            self.executable = None
            self.initialized = True

    def set_executable(self, exc):
        self.executable = exc

    def send_test_data(self, data):
        p = subprocess.Popen(self.executable, stdin=subprocess.PIPE, stdout=subprocess.PIPE)

        rx, _ = p.communicate(data)
        return self.parse_test_data(rx)

    def parse_test_data(self, data):
        data = data.strip().decode()

        objects = []
        offset = 0

        while offset < len(data):
            num = ''
            while offset < len(data) and data[offset].isdigit():
                num += data[offset]
                offset += 1

            json_len = int(num)
            json_str_end = min(offset + json_len, len(data))
            objects.append(
                json.loads(data[offset : json_str_end])
            )

            offset += json_len

        return objects


class ParserTests(unittest.TestCase):
    def setUp(self):
        self.test_program = TestExecutableMonostate()

    def send_test_data(self, data):
        return self.test_program.send_test_data(data)

    def test_simple_doc(self):
        obj = {'foo': 'bar'}
        self.assertEqual(self.send_test_data(zdumps(obj)), [obj])

    def test_two_simple_docs(self):
        obj = {'foo': 'bar'}
        test_str = zdumps(obj) * 2
        self.assertEqual(self.send_test_data(test_str), [obj] * 2)

    def test_brace_in_doc(self):
        obj = {'foo': '}'}
        test_str = zdumps(obj)
        self.assertEqual(self.send_test_data(test_str), [obj])

    def test_quote_in_doc(self):
        obj = {'foo': '"'}
        test_str = zdumps(obj)
        self.assertEqual(self.send_test_data(test_str), [obj])

    def test_multipart_not_implemented(self):
        test_str = b'{}-'
        self.assertEqual(self.send_test_data(test_str), [])

    def test_nested_object(self):
        obj = {
            'foo': 'bar',
            'your': {
                'mom': 1,
                'blah': 'blah',
                'blah': {},
                'cani': {'break': 'it', '?': '}"'},
            }
        }
        test_str = zdumps(obj)
        self.assertEqual(self.send_test_data(test_str), [obj])


if __name__ == '__main__':
    import sys

    language = sys.argv.pop(1)
    program = os.path.abspath(os.path.join(os.path.dirname(__file__), language, 'weebit'))
    TestExecutableMonostate().set_executable(program)

    unittest.main()
