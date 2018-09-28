import json
import os
import subprocess
import unittest


WEEBITC = os.path.abspath(os.path.join(os.path.dirname(__file__), 'weebitc'))
WEEBITCPP = os.path.abspath(os.path.join(os.path.dirname(__file__), 'weebitcpp'))


def dumps(obj):
    return json.dumps(obj).encode()


def zdumps(obj):
    return b'{}' + dumps(obj)


loads = json.loads


class CParserTests(unittest.TestCase):
    TEST_PROGRAM = WEEBITC

    def send_test_data(self, data):
        p = subprocess.Popen(self.TEST_PROGRAM, stdin=subprocess.PIPE, stdout=subprocess.PIPE)

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


class CPPParserTests(CParserTests):
    TEST_PROGRAM = WEEBITCPP


if __name__ == '__main__':
    unittest.main()
