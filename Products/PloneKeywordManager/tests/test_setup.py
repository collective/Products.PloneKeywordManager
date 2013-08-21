# -*- coding: utf-8 -*-

import unittest2 as unittest

from plone.app.testing import TEST_USER_ID
from plone.app.testing import setRoles
from plone.app.testing import logout
from Products.PloneKeywordManager.config import PROJECTNAME
from Products.PloneKeywordManager.testing import INTEGRATION_TESTING
from zope.component import getMultiAdapter
from Products.PloneKeywordManager.browser.interfaces import IPloneKeywordManagerLayer
from zope import interface
from zope.component.interfaces import ComponentLookupError


class InstallTestCase(unittest.TestCase):

    layer = INTEGRATION_TESTING

    def setUp(self):
        self.portal = self.layer['portal']
        self.request = self.layer['request']
        setRoles(self.portal, TEST_USER_ID, ['Manager'])
        self.markRequestWithLayer()

    def test_installed(self):
        qi = getattr(self.portal, 'portal_quickinstaller')
        self.assertTrue(qi.isProductInstalled(PROJECTNAME))

    def test_prefs_keywords_view(self):
        """
        test if the view is registered
        """
        view = getMultiAdapter((self.portal, self.request), name="prefs_keywords_view")
        view = view.__of__(self.portal)
        self.failUnless(view())

    def test_prefs_keywords_view_protected(self):
        """
        test if the view is protected
        """
        from AccessControl import Unauthorized
        logout()
        self.assertRaises(Unauthorized, self.portal.restrictedTraverse, '@@prefs_keywords_view')

    def markRequestWithLayer(self):
        # to be removed when p.a.testing will fix https://dev.plone.org/ticket/11673
        request = self.layer['request']
        interface.alsoProvides(request, IPloneKeywordManagerLayer)


class UninstallTestCase(unittest.TestCase):

    layer = INTEGRATION_TESTING

    def setUp(self):
        self.portal = self.layer['portal']
        self.request = self.layer['request']
        setRoles(self.portal, TEST_USER_ID, ['Manager'])
        self.qi = getattr(self.portal, 'portal_quickinstaller')
        self.qi.uninstallProducts(products=[PROJECTNAME])

    def test_uninstalled(self):
        self.assertFalse(self.qi.isProductInstalled(PROJECTNAME))

    def markRequestWithLayer(self):
        # to be removed when p.a.testing will fix https://dev.plone.org/ticket/11673
        request = self.layer['request']
        interface.alsoProvides(request, IPloneKeywordManagerLayer)

    def test_prefs_keywords_view_unregistered(self):
        """
        test if the view is not registered
        """
        self.assertRaises(AttributeError, self.portal.restrictedTraverse, '@@prefs_keywords_view')
