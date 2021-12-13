from plone import api
from Products.CMFPlone.interfaces import INonInstallable
from Products.PloneKeywordManager.compat import to_str
from zope.interface import implementer


@implementer(INonInstallable)
class HiddenProfiles:
    def getNonInstallableProfiles(self):
        """Hide uninstall profile from site-creation and quickinstaller."""
        return ["Products.PloneKeywordManager:uninstall"]


def importKeywords(context):
    """Create a document with an empty body to setup all keywords"""
    keywords = context.readDataFile("keywords.txt")
    if keywords is None:
        return

    keywords = to_str(keywords)
    keywordlist = [i for i in keywords.split("\n") if i]
    if len(keywordlist) < 1:
        return

    portal = context.getSite()
    cid = "keywords"
    doc = getattr(portal, cid, None)

    if doc is None:
        doc = api.content.create(
            container=portal, type="Document", id=cid, title="Subjects"
        )

    doc.setSubject(keywordlist)
    doc.exclude_from_nav = True
    doc.reindexObject()
