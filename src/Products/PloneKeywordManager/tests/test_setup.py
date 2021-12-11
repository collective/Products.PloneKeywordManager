"""Setup tests for this package."""
from plone import api
from plone.app.testing import setRoles
from plone.app.testing import TEST_USER_ID
from Products.CMFPlone.utils import get_installer
from Products.PloneKeywordManager.testing import PLONEKEYWORDMANAGER_INTEGRATION_TESTING

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
