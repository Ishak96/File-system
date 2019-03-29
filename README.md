# File Manager
file manager that uses a simulated partition (UNIX file), OS Project

## getting started
### installing
To install this project:
To download the project
```
	git clone https://www.github.com/ishak96/PROJET_OS
```
Then go to the directory
```
	cd PROJET_OS
```
Then create the bin and obj dir's
```
	mkdir obj bin
```
compile docs
```
make gendocs
```

To compile tests and shell interface
```
	make test
```

To run these tests
```
	./bin/<testname>
```

To run the shell interface
```
	./bin/shell <disk> [-f] [--format]
```