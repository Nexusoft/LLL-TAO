JULIA         ?= julia
JULIA_PROJECT := --project=.

.PHONY: test precompile sysimage clean

## Run the full test suite
test:
	$(JULIA) $(JULIA_PROJECT) test/runtests.jl

## Precompile the package (warm up the package cache)
precompile:
	$(JULIA) $(JULIA_PROJECT) -e 'using LLLTaoQTV'

## Build a system image with PackageCompiler (requires PackageCompiler.jl)
sysimage:
	$(JULIA) $(JULIA_PROJECT) -e 'using PackageCompiler; create_sysimage([:LLLTaoQTV], sysimage_path="LLLTaoQTV.so", precompile_execution_file="test/runtests.jl")'

## Remove build artifacts
clean:
	rm -f LLLTaoQTV.so
	rm -rf .julia_depot_sysimage
