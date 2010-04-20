import unittest

# some Python 2.3 unittest compatibility fixes
if not hasattr(unittest.TestCase, 'assertTrue'):
    unittest.TestCase.assertTrue = unittest.TestCase.failUnless
if not hasattr(unittest.TestCase, 'assertFalse'):
    unittest.TestCase.assertFalse = unittest.TestCase.failIf

mod_base = 'tests'


def suite():
    import os.path
    from glob import glob

    suite = unittest.TestSuite()

    for testcase in glob(os.path.join(os.path.dirname(__file__), 'test_*.py')):
        mod_name = os.path.basename(testcase).split('.')[0]
        full_name = '%s.%s' % (mod_base, mod_name)

        mod = __import__(full_name)

        for part in full_name.split('.')[1:]:
            mod = getattr(mod, part)

        suite.addTest(mod.suite())

    return suite


def main():
    unittest.main(defaultTest='suite')

if __name__ == '__main__':
    main()
