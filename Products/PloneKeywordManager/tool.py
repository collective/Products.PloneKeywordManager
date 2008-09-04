# Copyright (c) 2005 gocept gmbh & co. kg
# See also LICENSE.txt
# $Id$

# Python imports
try:
    import Levenshtein
    USE_LEVENSHTEIN = True
except ImportError:
    import difflib
    USE_LEVENSHTEIN = False

# Zope imports
import Globals
from OFS.SimpleItem import SimpleItem
from AccessControl import ClassSecurityInfo
from AccessControl import getSecurityManager
from AccessControl import Unauthorized
from Products.PageTemplates.PageTemplateFile import PageTemplateFile

# CMF imports
from Products.CMFCore.utils import UniqueObject, getToolByName
from Products.CMFCore.Expression import Expression
try:
    from Products.CMFCore import permissions
except ImportError:
    #BBB Use CMFCorePermissions from CMF 1.4.x (part of Plone 2.0.x) instead.
    from Products.CMFCore import CMFCorePermissions as permissions

# Sibling imports
from Products.PloneKeywordManager.interfaces import IPloneKeywordManager
from Products.PloneKeywordManager import config

class PloneKeywordManager(UniqueObject, SimpleItem):
    """A portal wide tool for managing keywords within Plone."""

    plone_tool = 1

    id = "portal_keyword_manager"
    meta_type = "Plone Keyword Manager Tool"
    security = ClassSecurityInfo()

    __implements__ = (IPloneKeywordManager,)

    manage_options = ({'label' : 'Overview', 'action' : 'manage_overview'},)

    security.declareProtected(permissions.ManagePortal, 'manage_overview')
    manage_overview = PageTemplateFile('www/explainTool', globals(),
            __name__='manage_overview')

    security.declarePublic('usingLevenshtein')
    def usingLevenshtein(self):
        """ Returns True iff Levenshtein is installed and will be used instead
        of difflib
        """
        return USE_LEVENSHTEIN

    security.declarePublic('change')
    def change(self, old_keywords, new_keyword, context=None):
        """Updates all objects using the old_keywords.

        Objects using the old_keywords will be using the new_keywords
        afterwards.

        Returns the number of objects that have been updated.
        """
        self._checkPermission(context)
        query = {'Subject': old_keywords}
        if context is not None:
            query['path'] = '/'.join(context.getPhysicalPath())
        querySet = self._query(**query)

        for item in querySet:
            obj = item.getObject()
            subjectList = list(obj.Subject())

            for element in old_keywords:
                while (element in subjectList) and (element <> new_keyword):
                    subjectList[subjectList.index(element)] = new_keyword

            # dedupe new Keyword list (an issue when combining multiple keywords)
            subjectList = list(set(subjectList))
            obj.setSubject(subjectList)
            obj.reindexObject(idxs=['Subject'])

        return len(querySet)

    security.declarePublic('delete')
    def delete(self, keywords, context=None):
        """Removes the keywords from all objects using it.

        Returns the number of objects that have been updated.
        """
        self._checkPermission(context)
        query = {'Subject': keywords}
        if context is not None:
            query['path'] = '/'.join(context.getPhysicalPath())
        querySet = self._query(**query)

        for item in querySet:
            obj = item.getObject()
            subjectList = list(obj.Subject())

            for element in keywords:
                while element in subjectList:
                    subjectList.remove(element)

            obj.setSubject(subjectList)
            obj.reindexObject(idxs=['Subject'])
        return len(querySet)

    security.declarePublic('getKeywords')
    def getKeywords(self, context=None):
        self._checkPermission(context)
        query = {}
        if context is not None:
            query['path'] = '/'.join(context.getPhysicalPath())

        subjects = {}
        for b in self._query(**query):
            for subject in b.Subject:
                subjects[subject] = True

        subjects = subjects.keys()
        subjects.sort()
        return subjects

    security.declarePublic('getScoredMatches')
    def getScoredMatches(self, word, possibilities, num, score, context=None):
        """ Take a word,
            compare it to a list of possibilities,
            return max. num matches > score).
        """
        self._checkPermission(context)
        if not USE_LEVENSHTEIN:
            # No levenshtein module around. Fall back to difflib
            return difflib.get_close_matches(word, possibilities, num, score)

        # Levenshtein is around, so let's use it.
        res = []

        # Search for all similar terms in possibilities
        for item in possibilities:
            lscore = Levenshtein.ratio(word, item)
            if lscore > score:
                res.append((item,lscore))

        # Sort by score (high scores on top of list)
        res.sort(lambda x,y: -cmp(x[1],y[1]))

        # Return first n terms without scores
        return [item[0] for item in res[:num]]

    def _query(self, **kwargs):
        catalog = getToolByName(self, 'portal_catalog')
        return catalog(**kwargs)

    def _checkPermission(self, context):
        if context is not None:
            context = context
        else:
            context = self
        if not getSecurityManager().checkPermission(
            config.MANAGE_KEYWORDS_PERMISSION, context):
            raise Unauthorized("You don't have the necessary permissions to "
                               "access %r." % (context,))

Globals.InitializeClass(PloneKeywordManager)
