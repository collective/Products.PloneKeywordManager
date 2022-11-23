"""Setup tests for this package."""
from plone import api
from plone.app.testing import setRoles
from plone.app.testing import TEST_USER_ID
from Products.CMFPlone.utils import get_installer
from Products.PloneKeywordManager.setuphandlers import importKeywords
from Products.PloneKeywordManager.testing import PLONEKEYWORDMANAGER_INTEGRATION_TESTING
from Products.PloneKeywordManager.upgrades import to_4

import unittest


class TestSetup(unittest.TestCase):
    """Test that Products.PloneKeywordManager is properly installed."""

    layer = PLONEKEYWORDMANAGER_INTEGRATION_TESTING

    def setUp(self):
        """Custom shared utility setup for tests."""
        self.portal = self.layer["portal"]
        self.installer = get_installer(self.portal, self.layer["request"])

    def test_product_installed(self):
        """Test if Products.PloneKeywordManager is installed."""
        self.assertTrue(
            self.installer.is_product_installed("Products.PloneKeywordManager")
        )

    def test_browserlayer(self):
        """Test that IPloneKeywordManagerLayer is registered."""
        from plone.browserlayer import utils
        from Products.PloneKeywordManager.browser.interfaces import (
            IPloneKeywordManagerLayer,
        )

        self.assertIn(IPloneKeywordManagerLayer, utils.registered_layers())

    def test_add_keywords_from_profile(self):
        """Check that the setuphandlers code imports the keywords."""

        class FakeContext:
            def readDataFile(self, filename):
                return "\n".join(new_keywords)

            def getSite(self):
                return api.portal.get()

        setRoles(self.portal, TEST_USER_ID, ["Manager"])

        new_keywords = ("apple", "pear", "pineapple", "cherries")
        self.assertNotIn("keywords", self.portal.objectIds())
        context = FakeContext()
        importKeywords(context)
        catalog = api.portal.get_tool("portal_catalog")
        self.assertIn("keywords", self.portal.objectIds())
        keywords_on_obj = self.portal.keywords.Subject()
        for keyword in new_keywords:
            self.assertIn(keyword, keywords_on_obj)

    def test_upgrade_setp_to_4(self):
        """Check that the persistent tool is removed"""
        self.portal
        setRoles(self.portal, TEST_USER_ID, ["Manager"])
        api.content.create(
            container=self.portal, type="Document", id="portal_keyword_manager"
        )
        to_4(self.portal.portal_setup)
        self.assertNotIn("portal_keyword_manager", self.portal.objectIds())


class TestUninstall(unittest.TestCase):

    layer = PLONEKEYWORDMANAGER_INTEGRATION_TESTING

    def setUp(self):
        self.portal = self.layer["portal"]
        self.installer = get_installer(self.portal, self.layer["request"])
        roles_before = api.user.get_roles(TEST_USER_ID)
        setRoles(self.portal, TEST_USER_ID, ["Manager"])
        self.installer.uninstall_product("Products.PloneKeywordManager")
        setRoles(self.portal, TEST_USER_ID, roles_before)

    def test_product_uninstalled(self):
        """Test if Products.PloneKeywordManager is cleanly uninstalled."""
        self.assertFalse(
            self.installer.is_product_installed("Products.PloneKeywordManager")
        )

    def test_browserlayer_removed(self):
        """Test that IPloneKeywordManagerLayer is removed."""
        from plone.browserlayer import utils
        from Products.PloneKeywordManager.browser.interfaces import (
            IPloneKeywordManagerLayer,
        )

        self.assertNotIn(IPloneKeywordManagerLayer, utils.registered_layers())
