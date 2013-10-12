
### Building ci

ci has been developed and testing on OS X 10.8 and 10.9.

You will need the latest version of Xcode (5.0).

ci depends on libcurl (pre-installed with OS X) and [boost](http://boost.org).

The build file and Xcode project assume that boost has been installed in /usr/local by something like homebrew.

If you are using homebrew make sure that you install boost with the --with-c++11 switch to avoid segmentation faults.

 
