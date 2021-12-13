from plone.app.contenttypes.testing import PLONE_APP_CONTENTTYPES_FIXTURE
from plone.app.testing import applyProfile
from plone.app.testing import FunctionalTesting
from plone.app.testing import IntegrationTesting
from plone.app.testing import PloneSandboxLayer


class PloneKeywordManagerLayer(PloneSandboxLayer):

    defaultBases = (PLONE_APP_CONTENTTYPES_FIXTURE,)

    def setUpZope(self, app, configurationContext):
        # Load any other ZCML that is required for your tests.
        # The z3c.autoinclude feature is disabled in the Plone fixture base
        # layer.
        import Products.PloneKeywordManager

        self.loadZCML(package=Products.PloneKeywordManager)

    def setUpPloneSite(self, portal):
        applyProfile(portal, "Products.PloneKeywordManager:default")


PLONEKEYWORDMANAGER_FIXTURE = PloneKeywordManagerLayer()


PLONEKEYWORDMANAGER_INTEGRATION_TESTING = IntegrationTesting(
    bases=(PLONEKEYWORDMANAGER_FIXTURE,),
    name="PloneKeywordManagerLayer:IntegrationTesting",
)


PLONEKEYWORDMANAGER_FUNCTIONAL_TESTING = FunctionalTesting(
    bases=(PLONEKEYWORDMANAGER_FIXTURE,),
    name="PloneKeywordManagerLayer:FunctionalTesting",
)
