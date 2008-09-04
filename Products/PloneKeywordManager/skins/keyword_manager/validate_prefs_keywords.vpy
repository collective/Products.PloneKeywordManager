## Controller Validator "validate_prefs_keywords"
##bind container=container
##bind context=context
##bind namespace=
##bind script=script
##bind state=state
##bind subpath=traverse_subpath
##parameters=
##title=Validating keywords form
##

from Products.CMFPlone import PloneMessageFactory as _

REQUEST= context.REQUEST

keywords = REQUEST.get('keywords', None)
if not keywords:
   state.setError('keywords', _(u'Please select at least one keyword'))

if REQUEST.form.has_key('form.button.Merge'):
   # We should assume there is a 'changeto' filled
   changeto = REQUEST.get('changeto', None)
   if not changeto:
      state.setError('changeto', _(u'Please provide a new term'))

if state.getErrors():
   # An error has occurred.  Set the status to 'failure' 
   # (the default initial status is 'success')
   context.plone_utils.addPortalMessage((u'Please correct the indicated errors.'), 'error')
   state.set(status='failure')

return state
