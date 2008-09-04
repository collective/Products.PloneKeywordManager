## Script (Python) "prefs_keywords_action_delete"
##bind container=container
##bind context=context
##bind namespace=
##bind script=script
##bind subpath=traverse_subpath
##parameters=keywords
##title=
##
from Products.CMFPlone import PloneMessageFactory as _

from Products.CMFCore.utils import getToolByName
pkm = getToolByName(context, "portal_keyword_manager")
changed_objects = pkm.delete(keywords, context=context.aq_inner)

msg = _(u"Deleted %s for %d object(s).") % (','.join(keywords), changed_objects)

context.plone_utils.addPortalMessage(msg)
return state
