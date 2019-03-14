# -*- coding: utf-8 -*-
from plone import api
from plone.app.testing import FunctionalTesting
from plone.app.testing import IntegrationTesting
from plone.app.testing import PLONE_FIXTURE
from plone.app.testing import PloneSandboxLayer
from plone.testing import z2


PLONE_5 = api.env.plone_version() >= "5"


class Fixture(PloneSandboxLayer):

    defaultBases = (PLONE_FIXTURE,)

    def setUpZope(self, app, configurationContext):
        # Load ZCML
        import Products.PloneKeywordManager
        import plone.app.dexterity

        self.loadZCML(package=plone.app.dexterity)
        try:
            import Products.ATContentTypes

            self.loadZCML(package=Products.ATContentTypes)
        except ImportError:
            pass
        self.loadZCML(package=Products.PloneKeywordManager)

        # Install product and call its initialize() function
        z2.installProduct(app, "Products.PloneKeywordManager")

    def setUpPloneSite(self, portal):
        # Install into Plone site using portal_setup
        self.applyProfile(portal, "Products.PloneKeywordManager:default")
        self.applyProfile(portal, "plone.app.dexterity:default")

        if PLONE_5:
            try:
                import Products.ATContentTypes  # noqa

                self.applyProfile(portal, "Products.ATContentTypes:default")
            except ImportError:
                pass


FIXTURE = Fixture()
INTEGRATION_TESTING = IntegrationTesting(
    bases=(FIXTURE,), name="Products.PloneKeywordManager:Integration"
)
FUNCTIONAL_TESTING = FunctionalTesting(
    bases=(FIXTURE,), name="Products.PloneKeywordManager:Functional"
)
