# Daepp
A parser and compiler for an extended gothic 2 script language.

Right now this is not a working prototype and the features are best enjoyed within a debugger.
See 'feature set' for more specific information.

## configuration
Inside the config.json you find a selection of parameters to configure the parser and language features.
Here you find explanations and recommendations as well as the *default value* for the normal gothic parser.
Settings of the section "language config" can be switched inside a .src file using a statement like this:
```
# caseSensitive true
```
* sourceDir is where your gothic.src is found
* outputDir the folder where the .dat is placed, using the same name as the provided .src

language config
* caseSensitive [*false*] case sensitivity is definitely incompatible with the default scripts!
* alwaysSemicolon [*true*] enforces semicolon at the end of {} segments. Setting it to false will make them optional, meaning that you can still parse old code.

compiler config
* saveInOrder [*true*] require the compiler to save all symbols sorted by there id. Is currently always true.
* showCodeSegmentOnError [*false*] When encounting an error the code segment in wich the error was found will be shown in the console.

## feature set
* parse known symbol types (var, function, class, prototype, instance)
* parse code (of functions)
* compile everything to a valid .dat

3rd party stuff used:

* [jofilelib](https://github.com/Jojendersie/JoFileLib)
* [easyloggingpp](https://github.com/easylogging/easyloggingpp)
* [dirent](http://pubs.opengroup.org/onlinepubs/9699919799/)
