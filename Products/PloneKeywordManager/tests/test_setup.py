# -*- coding: utf-8 -*-

from plone.app.testing import TEST_USER_ID
from plone.app.testing import setRoles
from plone.app.testing import logout
from Products.PloneKeywordManager.config import PROJECTNAME
from Products.PloneKeywordManager.tests.base import BaseIntegrationTestCase
from zope.component import getMultiAdapter


class InstallTestCase(BaseIntegrationTestCase):

    def test_installed(self):
        qi = getattr(self.portal, 'portal_quickinstaller')
        self.assertTrue(qi.isProductInstalled(PROJECTNAME))

    def test_prefs_keywords_view(self):
        """
        test if the view is registered
        """
        view = getMultiAdapter((self.portal, self.request), name="prefs_keywords_view")
        view = view.__of__(self.portal)
        self.assertTrue(view())

    def test_prefs_keywords_view_protected(self):
        """
        test if the view is protected
        """
        from AccessControl import Unauthorized
        logout()
        self.assertRaises(Unauthorized, self.portal.restrictedTraverse, '@@prefs_keywords_view')


class UninstallTestCase(BaseIntegrationTestCase):

    def setUp(self):
        self.portal = self.layer['portal']
        self.request = self.layer['request']
        setRoles(self.portal, TEST_USER_ID, ['Manager'])
        self.qi = getattr(self.portal, 'portal_quickinstaller')
        self.qi.uninstallProducts(products=[PROJECTNAME])

    def test_uninstalled(self):
        self.assertFalse(self.qi.isProductInstalled(PROJECTNAME))
