from setuptools import setup, find_packages

version = '2.2.2'

long_description = open("README.rst").read() + "\n" + \
                   open("CHANGES.rst").read()


setup(name='Products.PloneKeywordManager',
      version=version,
      description="Plone Keyword Manager allows you to change, merge and delete \
          keywords in Plone and updates all corresponding objects automatically.\
          It uses a similiarity search to support you in identifying similar keywords.\
          Keywords can be cleaned up from time to time by a site manager to \
          create a consistent vocabulary.",
      long_description=long_description,
      classifiers=[
        "Development Status :: 6 - Mature",
        "Environment :: Web Environment",
        "Framework :: Plone",
        "Framework :: Plone :: 4.2",
        "Framework :: Plone :: 4.3",
        "Framework :: Plone :: 5.0",
        "Intended Audience :: End Users/Desktop",
        "Intended Audience :: System Administrators",
        "License :: OSI Approved :: GNU General Public License (GPL)",
        "Operating System :: OS Independent",
        "Programming Language :: Python",
        "Programming Language :: Python :: 2.7",
        "Topic :: Software Development :: Libraries :: Python Modules",
        ],
      keywords='plone keywords',
      author='Michael Dunlap',
      author_email='dunlapm@u.washington.edu',
      url='https://github.com/collective/Products.PloneKeywordManager',
      license='GPL',
      packages=find_packages(exclude=['ez_setup']),
      namespace_packages=['Products'],
      include_package_data=True,
      zip_safe=False,
      install_requires=[
        'setuptools',
        'plone.api',
        ],
      extras_require={
        'Levenshtein': ['python-Levenshtein'],
        'test': [
          'plone.app.testing',
          'plone.app.dexterity',
          'Products.ATContentTypes',
          ],
        },
      entry_points="""
      [z3c.autoinclude.plugin]
      target = plone
      """,
      )
