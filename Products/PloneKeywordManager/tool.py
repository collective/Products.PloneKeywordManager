# Copyright (c) 2005 gocept gmbh & co. kg
# See also LICENSE.txt
# $Id: tool.py 47645 2007-08-20 14:59:10Z glenfant $

import pkg_resources

# Python imports
try:
    import Levenshtein
    USE_LEVENSHTEIN = True
except ImportError:
    import difflib
    USE_LEVENSHTEIN = False

# multiligual detection
try:
    pkg_resources.get_distribution('plone.app.multilingual')
    MULTILINGUAL = True
except pkg_resources.DistributionNotFound:
    try:
        pkg_resources.get_distribution('Products.LinguaPlone')
        MULTILINGUAL = True
    except pkg_resources.DistributionNotFound:
        MULTILINGUAL = False

# Zope imports
try:
    from App.class_init import InitializeClass
except ImportError:  # < Zope 2.13
    from Globals import InitializeClass

from Acquisition import aq_base
from OFS.SimpleItem import SimpleItem
from AccessControl import ClassSecurityInfo
from AccessControl import getSecurityManager
from AccessControl import Unauthorized
from Products.PageTemplates.PageTemplateFile import PageTemplateFile
from zope import interface

# CMF imports
from Products.CMFCore.utils import getToolByName
from Products.CMFCore.utils import UniqueObject
from Products.CMFCore import permissions as CMFCorePermissions

# Sibling imports
from Products.PloneKeywordManager.interfaces import IPloneKeywordManager
from Products.PloneKeywordManager import config


class PloneKeywordManager(UniqueObject, SimpleItem):
    """A portal wide tool for managing keywords within Plone."""

    plone_tool = 1

    id = "portal_keyword_manager"
    meta_type = "Plone Keyword Manager Tool"
    security = ClassSecurityInfo()

    interface.implements(IPloneKeywordManager)

    manage_options = ({'label': 'Overview', 'action': 'manage_overview'},)

    security.declareProtected(CMFCorePermissions.ManagePortal, 'manage_overview')
    manage_overview = PageTemplateFile(
        'www/explainTool', globals(), __name__='manage_overview')

    security.declarePublic('change')

    def _getFullIndexList(self, indexName):
        idxs = set([indexName]).union(config.ALWAYS_REINDEX)
        return list(idxs)

    def change(self, old_keywords, new_keyword, context=None, indexName='Subject'):
        """Updates all objects using the old_keywords.

        Objects using the old_keywords will be using the new_keywords
        afterwards.

        Returns the number of objects that have been updated.
        """
        self._checkPermission(context)
        # #MOD Dynamic field getting
        query = {indexName: old_keywords}
        if context is not None:
            query['path'] = '/'.join(context.getPhysicalPath())
        if MULTILINGUAL:
            query['Language'] = 'all'
        querySet = self._query(**query)
        for item in querySet:
            obj = item.getObject()
            # #MOD Dynamic field getting
            subjectList = self.getListFieldValues(obj, indexName)

            for element in old_keywords:
                while (element in subjectList) and (element != new_keyword):
                    subjectList[subjectList.index(element)] = new_keyword

            # dedupe new Keyword list (an issue when combining multiple keywords)
            subjectList = list(set(subjectList))

            # #MOD Dynamic field update
            updateField = self.getSetter(obj, indexName)
            if updateField is not None:
                updateField(subjectList)
                idxs = self._getFullIndexList(indexName)
                obj.reindexObject(idxs=idxs)

        return len(querySet)

    security.declarePublic('delete')
    def delete(self, keywords, context=None, indexName='Subject'):
        """Removes the keywords from all objects using it.

        Returns the number of objects that have been updated.
        """
        self._checkPermission(context)
        # #Mod Dynamic field
        query = {indexName: keywords}
        if context is not None:
            query['path'] = '/'.join(context.getPhysicalPath())
        if MULTILINGUAL:
            query['Language'] = 'all'
        querySet = self._query(**query)

        for item in querySet:
            obj = item.getObject()

            subjectList = self.getListFieldValues(obj, indexName)

            for element in keywords:
                while element in subjectList:
                    subjectList.remove(element)

            updateField = self.getSetter(obj, indexName)
            if updateField is not None:
                updateField(subjectList)
                idxs = self._getFullIndexList(indexName)
                obj.reindexObject(idxs=idxs)

        return len(querySet)

    security.declarePublic('getKeywords')
    def getKeywords(self, context=None, indexName='Subject'):
        self._checkPermission(context)
        if indexName not in self.getKeywordIndexes():
            raise ValueError, "%s is not a valid field" % indexName

        catalog = getToolByName(self, 'portal_catalog')

        # why work hard if we don't have to?
        # if hasattr(catalog,'uniqueValuesFor'):
        keywords = list(catalog.uniqueValuesFor(indexName))
        # else:
        #    query = {}
        #    if context is not None:
        #        query['path'] = '/'.join(context.getPhysicalPath())
        #    keywords = {}
        #    for b in self._query(**query):
        #        for keyword in getattr(b,indexName)():
        #            keywords[keyword] = True
        #    keywords = keywords.keys()

        keywords.sort()
        return keywords

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
        if isinstance(word, str):
            oword = unicode(word, 'utf-8')
        else:
            oword = word.encode('utf-8')

        for item in possibilities:
            if isinstance(item, type(word)):
                lscore = Levenshtein.ratio(word, item)
            elif isinstance(item, type(oword)):
                lscore = Levenshtein.ratio(oword, item)
            else:
                raise ValueError, "%s is not a normal, or unicode string" % item
            if lscore > score:
                res.append((item, lscore))

        # Sort by score (high scores on top of list)
        res.sort(lambda x, y: -cmp(x[1], y[1]))

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
                               "access %r." % context)

    def getKeywordIndexes(self):
        """Gets a list of indexes from the catalog. Uses config.py to choose the
        meta type and filters out a subset of known indexes that should not be
        managed.
        """
        catalog = getToolByName(self, 'portal_catalog')
        idxs = catalog.index_objects()
        idxs = [i.id for i in idxs if i.meta_type == config.META_TYPE and
                i.id not in config.IGNORE_INDEXES]
        idxs.sort()
        return idxs

    security.declarePrivate('fieldNameForIndex')
    def fieldNameForIndex(self, indexName):
        """The name of the index may not be the same as the field on the object, and we need
           the actual field name in order to find its mutator.
        """
        catalog = getToolByName(self, 'portal_catalog')
        indexObjs = [idx for idx in catalog.index_objects() if idx.getId() == indexName]
        try:
            fieldName = indexObjs[0].indexed_attrs[0]
        except IndexError:
            raise ValueError('Found no index named %s' % indexName)

        return fieldName

    security.declarePrivate('getSetter')
    def getSetter(self, obj, indexName):
        """Gets the setter function for the field based on the index name.

        Returns None if it can't get the function
        """
        # Archetypes:
        if getattr(aq_base(obj), 'getField', None) is not None:
            fieldName = self.fieldNameForIndex(indexName)
            fieldObj = obj.getField(fieldName) or obj.getField(fieldName.lower())
            if not fieldObj and fieldName.startswith('get'):
                fieldName = fieldName.lstrip('get')
                fieldName = fieldName[0].lower() + fieldName[1:]
                fieldObj = obj.getField(fieldName)
            if fieldObj is not None:
                return fieldObj.getMutator(obj)
            return None
        # DefaultDublinCoreImpl:
        setterName = 'set' + indexName
        if getattr(aq_base(obj), setterName, None) is not None:
            return getattr(obj, setterName)

        return None

    security.declarePrivate('getListFieldValues')
    def getListFieldValues(self, obj, indexName):
        """Returns the current values for the given Lines field as a list.
        """
        fieldName = self.fieldNameForIndex(indexName)
        fieldVal = getattr(obj, fieldName, ())
        if not fieldVal and fieldName.startswith('get'):
            fieldName = fieldName.lstrip('get')
            fieldName = fieldName[0].lower() + fieldName[1:]
            fieldVal = getattr(obj, fieldName, ())
        if callable(fieldVal):
            return list(fieldVal())
        else:
            return list(fieldVal)


InitializeClass(PloneKeywordManager)
