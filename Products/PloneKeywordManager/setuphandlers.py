#from Products.CMFCore.utils import getToolByName
from Products.PloneKeywordManager import HAS_DEXTERITY
if HAS_DEXTERITY:
    from plone.dexterity.interfaces import IDexterityContent
else:
    class IDexterityContent(object):
        pass


def importKeywords(context):
    """Create a document with an empty body to setup all keywords"""
    keywords = context.readDataFile('keywords.txt')
    if keywords is None:
        return

    keywordlist = [i for i in keywords.split('\n') if i]
    if len(keywordlist) < 1:
        return

    site = context.getSite()
    id = 'keywords'
    doc = getattr(site, id, None)

    if doc is None:
        site.invokeFactory('Document', id, title='Keywords')
        doc = getattr(site, id)

    doc.setSubject(keywordlist)
    if hasattr(IDexterityContent, 'providedBy') and IDexterityContent.providedBy(doc):
        doc.exclude_from_nav = True
    else:
        doc.setExcludeFromNav(True)
    doc.reindexObject()
    doc.unmarkCreationFlag()
