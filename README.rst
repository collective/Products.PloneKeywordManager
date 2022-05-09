


.. image:: https://github.com/collective/Products.PloneKeywordManager/actions/workflows/CI.yml/badge.svg
    :alt: Github Testing Badge
    :target: https://github.com/collective/Products.PloneKeywordManager/actions/workflows/CI.yml

.. image:: https://coveralls.io/repos/collective/Products.PloneKeywordManager/badge.png?branch=master
    :alt: Coveralls badge
    :target: https://coveralls.io/r/collective/Products.PloneKeywordManager

.. image:: https://img.shields.io/badge/code%20style-black-000000.svg
    :target: https://github.com/ambv/black
    :alt: Code style: black


Plone Keyword Manager
=====================

Plone Keyword Manager allows you to change, merge and delete keywords (aka tags or subjects) in Plone and updates all corresponding objects automatically.
It uses a similarity search to support you in identifying similar keywords.
PloneKeywordManager helps you to build an inductive vocabulary with several people working on the same Plone site.
Keywords can be cleaned up from time to time by a site-manager to create a consistent vocabulary.

Plone Keyword Manager is a quite simple solution to a major problem in the real world use of Plone:
If you can't work with restricted vocabularies, your keyword-vocabulary will get duplicate entries very quickly - depending on your authors' interpretation of existing keywords.


Installation
============

In your buildout add ``Products.PloneKeywordManager`` to your instances eggs section or policy packages ``setup.py``.
If you want similarity search add ``Products.PloneKeywordManager[Levenshtein]`` instead.

Run buildout.
Activate it at Site Setups Add Ons page.


Usage
=====

After installing, you will find an entry in Site Setup a section called Keyword Manager.
Inside, you will see an alphabetical listing and a selection for all keywords existing in your site.

Use the last one to see similar terms for a single keyword.
Use the former one if you want to see a list of all keywords starting with letter 'b', click it.
The Plone Keyword Manager will then search all keywords starting with 'b' and will also look for similar keywords.
You can now select several keywords and delete them for example.
If you only want to change a single keyword, select it, then enter the new keyword and click on merge.
If you want to merge several keywords into one new one, select them, enter the new keyword and click on merge.

The last selected keyword is entered automatically into the textbox if JavaScript is enabled.
This may be irritating at first glance, but you'll learn to appreciate quite fast.
If you use it the right way, you don't have to copy&paste into the textbox.
Try it yourself, you'll get the idea behind it...


For developers and integrators
==============================

You can also use KeywordManager to import your keywords with GenericSetup.
You must have access to the file system to do this.

* Add a file ``keywords.txt`` to the directory ``Products/PloneKeywordManager/profiles/default``, with one keyword per line.

* In the ZMI, go to portal_setup, Import tab. Locate the step called ``Create or update keywords``. Check the box next to it, then click the button ``Import Selected Steps``.

This will add or update a Document titled ``Keywords`` (with ID ``keyword``) in the root of the Plone site;
this document will have all the listed keywords assigned to it.

Leave this document in the private state so only an administrator will be able to see and edit it.

Each time you run this import step, the ``Keywords`` document's keywords will be completely replaced with the contents of the ``keywords.txt`` file.



Source Code
===========

Source code and issue tracker of this project is in the
`Plone Collective <https://github.com/collective/Products.PloneKeywordManager>`_

For instructions how to contribute please read the `Collective Information Page <http://collective.github.io//>`_

Further development focuses on Plone 5.2 on Python 3.7 and higher.

Credits
=======

PloneKeywordManager was mainly coded by `Maik Jablonski <mailto:maik.jablonski@uni-bielefeld.de>`_ during the Plone Paderborn Sprint (back in September 2003),
founded by the Bertelsmann Foundation. First user interface updates and setup code by Alexander Limi from Plone Solutions.
Joe Geldart from `Netalley Networks <http://www.netalleynetworks.com>`_ updated the template to Plone 2.0 format.
We would also like to thank Maik for letting us put this code in the `Collective <http://collective.github.com/>`_ - so it can be improved and expanded easily.

After this, many people updated the code, templates and setup to follow up with the Plone releases.

These days the add-on is developed within the Plone Collective by a `group of volunteers <https://github.com/collective/Products.PloneKeywordManager/graphs/contributors>`_.
