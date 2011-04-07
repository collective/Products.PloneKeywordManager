from Testing import ZopeTestCase as ztc

from Products.PloneTestCase import PloneTestCase as ptc
from Products.PloneTestCase.layer import onsetup

# When ZopeTestCase configures Zope, it will *not* auto-load products in Products/.
# Instead, we have to use a statement such as:
#
#   ztc.installProduct('SimpleAttachment')
#
# This does *not* apply to products in eggs and Python packages (i.e. not in the Products.*) namespace.
# For that, see below.
#
# All of Plone's products are already set up by PloneTestCase.


@onsetup
def setup_product():
    """
    Set up the package and its dependencies.

    The @onsetup decorator causes the execution of this body to be deferred
    until the setup of the Plone site testing layer. We could have created
    our own layer, but this is the easiest way for Plone integration tests.
    """
    ztc.installProduct('PloneKeywordManager')

# The order here is important:
# We first call the (deferred) function which installs the products we need for this product.
# Then, we let PloneTestCase set up this product on installation.

setup_product()
ptc.setupPloneSite(products=['PloneKeywordManager'])


class PloneKeywordManagerTestCase(ptc.PloneTestCase):
    """
    We use this base class for all the tests in this package.
    If necessary, we can put common utility or setup code in here.
    This applies to unit test cases.
    """


class PloneKeywordManagerFunctionalTestCase(ptc.FunctionalTestCase):
    """
    We use this class for functional integration tests that use doctest syntax.
    Again, we can put basic common utility or setup code in here.
    """
