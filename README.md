# Sparc MPI Version

working in progress

```
	git clone https://github.com/Lizhen0909/sparc-mpi.git
	cd sparc-mpi && git submodule update --init --recursive
```

```
	cd extlib/mrmpi-7Apr14/src/ && make
	cd extlib/Mimir/ && ./autogen.sh && ./configure && make
```

```
	./configure --no_jvm --no_mpi --vid32
```