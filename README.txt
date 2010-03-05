Plone Keyword Manager
=====================

Plone Keyword Manager allows you to change, merge and delete
keywords in Plone and updates all corresponding objects automatically.
It uses a similiarity search to support you in identifying similar
keywords. PloneKeywordManager helps you to build an inductive vocabulary
with several people working on the same Plone site. Keywords can be cleaned
up from time to time by a site-manager to create a consistent vocabulary.

Plone Keyword Manager is a quite simple solution to a major problem in the real
world use of Plone: If you can't work with restricted vocbularies, your
keyword-vocabulary will get duplicate entries very quickly - depending on your
authors' interpretation of existing keywords.

Installation
============

Plone Keyword Manager can be easily installed and removed with the
built-in install mechanism in Plone 2 and up. Place it in your Products
directory and use the Add/Remove Products panel to activate.

If you want to install in a CMF site or a Plone 1 site,
it installs like any other CMF Product.

Plone Keyword Manager uses Python's difflib for the similarity search per
default, but if you want to speed up the PloneKeywordManager, you should
install the
"Python-Levenshtein-Module":http://trific.ath.cx/resources/python/levenshtein
(written in C) which will makes things faster up to 10 times. A copy of
the Levensthein-Module is provided in the package (to install it on linux do
something like "cd python-Levenshtein; python setup.py install").
Plone Keyword Manager will use the faster Levensthein-search automatically
if it can be imported by your Zope-Server.

Usage
=====

After installing, you will find an entry in Plone Setup called Keyword Manager.
Inside, you will see an alphabetical listing and a selection for all keywords
existing in your site.

Use the last one to see similar terms for a single keyword.
Use the former one if you want to see a list of all keywords starting with
letter 'b', click it. The Plone Keyword Manager will then search all keywords
starting with 'b' and will also look for similar keywords. You can now select
several keywords and delete them for example. If you only want to change a single
keyword, select it, then enter the new keyword and click on merge. If you want to
merge several keywords into one new one, select them, enter the new keyword
and click on merge.

The last selected keyword is entered automatically into the textbox if JavaScript
is enabled. This may be irritating at first glance, but you'll learn to appreciate
quite fast. If you use it the right way, you don't have to copy&paste into the
textbox. Try it yourself, you'll get the idea behind it...

For developer and integrator
============================

You can also use KeywordManager to import your keywords with genericsetup. Just
add a 'keywords.txt' in your profile with one keyword per line. This step add or
update a Document with an id to 'keywords'. Let it in private state so only
the administrator will be able to manage it.

Credits
=======

PloneKeywordManager was mainly
coded by Maik Jablonski during the Plone Paderborn Sprint (September 2003),
founded by the Bertelsmann Foundation.

Main code -- "Maik Jablonski":mailto:maik.jablonski@uni-bielefeld.de

User Interface updates and Setup Code -- Alexander Limi from
"Plone Solutions":http://www.plonesolutions.com

Thanks to Joe Geldart from
"Netalley Networks":http://www.netalleynetworks.com
for updating the template to Plone 2.0 format.

We would also like to thank Maik for letting us put this code in the
"Collective":http://plone.org/collective - so it can be improved and
expanded by the Collective developers.
