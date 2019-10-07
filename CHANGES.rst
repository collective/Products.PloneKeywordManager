Changelog
=========

2.2.2 (2019-10-07)
------------------

- Filter out empty keywords before sorting.
  Otherwise the control panel is broken for some indexes.
  Fixes `issue #28 <https://github.com/collective/Products.PloneKeywordManager/issues/28>`_.
  [maurits]


2.2.1 (2018-07-09)
------------------

- Add translation to Brazilian Portuguese.
  [hvelarde]

- Fix searchlink button to respect the selected index field in the query
  and quote the value for the search parameter
  [petschki]


2.2.0 (2016-03-07)
------------------

- accessibility improvements
  [daniele-andreotti]

- add compatibility with Plone 5
  [cewing]

- Drop support for Python 2.6.
  [hvelarde]


2.1.1 (2014-09-15)
------------------

- Don't break when searching encoded indexes. [davisagli]

- Fix ``getSetter`` method in tool.py to handle also discussion items. [cekk]

2.1 (2014-04-27)
----------------

- Enable search for values (search icon behind value). [jensens]

- Decode values so they work with Dexterity. [davisagli]

- Handle sets as field values. [davisagli]

- Update Dexterity content even if it has no explicit setter. [davisagli]

- Works when the field related to KeywordIndex is monovalued [thomasdesvenain]

- UI and labels improvements [thomasdesvenain]
	- selected values are kept after form submission
	- change in selection lists automatically submit the form
	- keywords are clickable labels
	- added some helpers and tooltips, and improved labels

- check for plone.app.multilingual and Products.LinguaPlone [pbauer]

- Fixed getSetter and getListFieldValues method in tool to handle also
  non-default field names [cekk]

- Removed skins and moved control panel to a browser view [cekk]

- Added custom permission to access the view [cekk]

- Fixed uninstall profile [cekk]

- Moved translations from i18n to locales [cekk]

2.0 (2013-04-24)
----------------

- take plone.app.multilingual into acount and set Language=all on change.
  [jensens]

- Use png icon as gif icon has been removed.
  [thomasdesvenain]

- Fix index update on keyword delete not to reindex all attributes. [leorochael]

- Add support for dexterity (and generic DublinCoreImpl subclasses). [leorochael]

- Tests use now plone.app.testing; test coverage improved. [hvelarde]

- Fix package distribution. [hvelarde]

- Do not create a `keywords` Document in the plone-site in case the ``keywords.txt``
  file is empty. (useful for sites not having Document globally allowed)
  [fRiSi]

- Allow `Site Administrators` too access the keyword managment
  [fRiSi]

- Make installation possible for dexterity-based Documents [pysailor]

1.9 (2011-06-22)
----------------

- Fixed critical error at index update.
  [thomasdesvenain]

- Upgrade imports for Zope 2.13. Remove deprecation warnings.
  [toutpt]

1.8 (2011-04-08)
----------------

- Add tests related to skins-directories. [WouterVH]

- remove old-style Install.py. [WouterVH]

- Add uninstall-profile. [WouterVH]

- Fix marker-file for setuphandlers.py. [WouterVH]

- Add MANIFEST.in to include docs in release. [WouterVH]

- Upgrade imports for Zope 2.13. Remove deprecation warnings.
  [thomasdesvenain]

- Manage python-Levenshtein dependency as a setuptools extras_require.
  [thomasdesvenain]

- French translation.
  [thomasdesvenain]

- Internationalization fixes.
  [thomasdesvenain]

- Added the z3c.autoinclude entry point so this package is automatically loaded
  on Plone 3.3 and above. [WouterVH]

- Remove the old-style refresh.txt and version.txt. Version is now specified in
  setup.py [WouterVH]

- Cleanup install-instructions. [WouterVH]


1.7 (20/08/2010)
----------------

- Added ability to mix unicode and non-unicode keywords and changes.
  Fixes a bug with collective.dancing (and plone.z3cform) upgrading
  form inputs to unicode automagically.
  [dunlapm]

- Restify the CHANGES.txt file.
  [toutpt]

- Add keywords import through genericsetup.
  [toutpt]

- Add a default profile based on Extensions/Install.py.
  [toutpt]

- remove zope2 interface.
  [toutpt]


1.6 (19/03/2009)
----------------

- Fixed handling of non-ASCII Keywords in Controller Python Scripts
  prefs_keywords_action_change.cpy and prefs_keywords_action_delete.cpy [disko]

- Added tests for the above mentioned bugfixes. [disko]

- Added German translation. [disko]


1.6b2 (15-11-2008)
------------------

- No longer assumes that the index name is the same as the name of the
  underlying schema field. [jessesnyder]


1.6b1 (09-09-2008)
------------------

- Eggification from PloneKeywordManager into Products.PloneKeywordManager. This
  package is only supported for Plone 3 now. It may or may not work in Plone 2.5. [dunlapm]

- Added support for multiple keyword indexes. If you have more than one keyword
  field on your content type(s) then you will still be able to manage all of your
  keywords. If you only use the single default field then you will get the normal
  interface.


1.5-alpha1 (28-11-2007)
-----------------------

- Plone 2.5 and Plone 3 compatibility for product PloneKeywordManager. [glenfant]


0.4 (unknown)
-------------

- Added Brazilian Portuguese i18n support.
  [Rafahela Bazzanella <rafabazzanella@yahoo.com.br>]


0.3 (05-04-2005)
-----------------

- Refactored code to run from a portal tool.

- Minor clean ups.

- Introduced the permission "Manage Keywords" to have better control about who
  can manage keywords.
