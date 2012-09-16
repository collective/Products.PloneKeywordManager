import unittest
from Products.PloneKeywordManager.tests.base import PloneKeywordManagerTestCase

SKINSDIRS = ['keyword_manager', ]


class TestSetup(PloneKeywordManagerTestCase):
    """Test the installation of this package
    Important: the name of all test-methods should start with test_
    """

    def afterSetUp(self):
        self.loginAsPortalOwner()

    def test_skinsdir_presence(self):
        #check the presence of the skins-directory in portal_skins
        for skinsdir in SKINSDIRS:
            self.failUnless(self.portal.portal_skins.hasObject(skinsdir),
                '%s-skinsdir is missing from portal_skins' % skinsdir)

    def test_skinsdir_in_active_theme(self):
        #check if skins-dir is listed in skins of the active theme
        active_skin = self.portal.portal_skins.getDefaultSkin()
        skinsfolder = self.portal.portal_skins.getSkinByName(active_skin)
        for skinsdir in SKINSDIRS:
            self.failUnless(skinsdir in skinsfolder.absolute_url_path(),
                '%s-skinsdir is not listed in the active theme' % skinsdir)

    def test_skinsdir_is_not_empty(self):
        #check if skins-dir contains items
        for skinsdir in SKINSDIRS:
            directory_view = self.portal.portal_skins.get(skinsdir, None)
            self.failUnless(directory_view,
                '%s filesystem directory view not found in portal_skins' % skinsdir)
            skin_objects = directory_view.objectItems()
            self.failUnless(skin_objects, '%s-skinsdir seems to empty' % skinsdir)

    def testFail(self):
        #always passing dummy tests
        self.failIf(False)
        self.failUnless(True)


def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestSetup))
    return suite
