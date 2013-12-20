ci - Continuous integration command tool
==

Practicing TDD and working with a Continuous Integration system is great but context switching between the command line
to check the status of my builds is annoying - enter *ci* a tool for checking the status of your builds right in the
command line.

Basic usage ci --server http://jenkins.domain.com

ci takes configuration parameters on the command line or configuration files places on the current path.


Take a look at the [github pages](http://grahambrooks.github.io/ci/) for this project.

Configuration
-------------

ci uses a set of configuraiton files to know what to do. It assumes that you have some sort of project hierachy on your filesystem.

ci assumes that you have a filesystem something like

<pre>

projects
       `- project1
       `- project2
</pre>

Assuming that project1 and project2 are built by the same ci system you will want to define that connection by having a .ci file in the projects folder.

.ci files are currently in json format and should not be checked in to version control because they contain continuous integration server credentials.

<pre>
{
    "server" : {
	"type" : "jenkins",
	"url" : "http://jenkins.example.com",
	"username" : "graham@grahambrooks.com",
	"password" : "my password"
    }
}
</pre>

Hudson and Jenkins are currently supported server types.

<pre>
> ci
</pre>



[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/grahambrooks/ci/trend.png)](https://bitdeli.com/free "Bitdeli Badge")

