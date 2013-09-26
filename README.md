ci
==

Continuous integration command tool

Practicing TDD and working with a Continuous Integration system is great but context switching between the command line to check the status of my builds is annoying - enter *ci* a tool for checking the status of your builds right in the command line.

Configuration
-------------

ci uses a set of configuraiton files to know what to do. It assumes that you have some sort of project hierachy on your filesystem.

ci assumes that you have a filesystem something like

<pre>

projects
       `- project1
       `- project2
</pre>

Assuming that project1 and project2 are built by the same ci system you will want to define that connection by having a .ci file in the projects folder. Optionally you can define a .ci file in the project folder so that ci only checks the the builds relevant to that project.

.ci files are currently in json format and should not be checked in to version control because they contain continuous integration server credentials.

Hudson and Jenkins are currently supported server types.

Setup
-----

Your CI server should require authentication so first you will need to get hold of the API token availalbe from the server on your account profile page.

ci self configures when run in a project file and no configuration can be found.

<pre>
> ci
</pre>

