import unittest
from Products.PloneKeywordManager.tests.base import PloneKeywordManagerTestCase

from Products.CMFCore.utils import getToolByName

class TestNonAsciiKeywords(PloneKeywordManagerTestCase):
    """The name of the class should be meaningful. This may be a class that
    tests the installation of a particular product.
    """
    
    def afterSetUp(self):
        """
        This method is called before each single test. It can be used to set up common state.
        Setup that is specific to a particular test should be done in that test method.
        """
        self.pkm = getToolByName(self.portal, 'portal_keyword_manager')
        self.setRoles(
            (
                'Manager',
            )
        )
        self.portal.setSubject(
            [
                u'Fr\\xfchst\\xfcck'.decode('utf-8'),
                'Mitagessen',
                'Abendessen',
            ]
        )
    
    def beforeTearDown(self):
        """
        This method is called after each single test. It can be used for cleanup, if you need it.
        Note that the test framework will roll back the Zope transaction at the end of each test,
        so tests are generally independent of one another.
        However, if you are modifying external resources (say a database) or globals
        (such as registering a new adapter in the Component Architecture during a test),
        you may want to tear things down here.
        """
        self.setRoles(
            ()
        )
    
    def _action_change(self, keywords, changeto, field='Subject'):
        """ calls prefs_keywords_action_change.py """
        self.portal.prefs_keywords_action_change(keywords, changeto, field)
    
    def _action_delete(self, keywords, field='Subject'):
        """ calls prefs_keywords_action_delete.cpy """
        self.portal.prefs_keywords_action_delete(keywords, field)
    
    # def test_show_non_ascii_bugs(self):
    #     """
    #     this demonstrates two bugs in the UI, which appear only with non-ASCII keywords.
    #     it is commented out, as will obviously fail after the bugfixes.
    #     """
    #     self.assertRaises(UnicodeDecodeError, self._action_change, [u'Fr\\xfchst\\xfcck'.decode('utf-8'), 'Mittagessen', ], 'Abendessen', )
    #     self.assertRaises(UnicodeDecodeError, self._action_delete, [u'Fr\\xfchst\\xfcck'.decode('utf-8'), ])
    
    def test_pref_keywords_action_change(self):
        """ test the bugfix for prefs_keywords_action_change """
        try:
            self._action_change([u'Fr\\xfchst\\xfcck'.decode('utf-8'), 'Mittagessen', ], 'Abendessen')
        except UnicodeDecodeError:
            self.fail()
    
    def test_pref_keywords_action_delete(self):
        """ test the bugfix for prefs_keywords_action_delete """
        try:
            self._action_delete([u'Fr\\xfchst\\xfcck'.decode('utf-8'), ])
        except UnicodeDecodeError:
            self.fail()
    
    # Keep adding methods here, or break it into multiple classes or multiple files as appropriate.
    # Having tests in multiple files makes it possible to run tests from just one package:
    #
    #   ./bin/instance test -s Products.PloneKeywordManager -t test_non_ascii_keywords


def test_suite():
    """
        This sets up a test suite that actually runs the tests
        in the class above.
    """
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestNonAsciiKeywords))
    return suite
