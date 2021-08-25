# Runtime-Env (Under construction - To be released in Fall 2021)
This is a [core Flight Systsem (cFS)](https://github.com/nasa/cFE) distribution that serves as the basis for other OpenSatKit distributions and as a starting point for anyone interested in **learning the cFS, starting their own cFS-based project, and developing, integrating, and testing new cFS applications**.

This distribution uses [CCSDS Electronic Datasheets (EDS)](https://public.ccsds.org/Pubs/876x0b1.pdf) for application interface definitions. These definitions are propagated via a toolcahin to produce artifacts that are used during the system build, integration, and deployment processes. Please see the [OSK Runtime Environment Wiki](https://github.com/OpenSatKit/Runtime-Env/wiki) for more details.

![EDS Workflow](https://github.com/OpenSatKit/Runtime-Env/blob/main/docs/img/eds-workflow.png)

# Getting Started

## Prerequisites

The system can be developed on any GNU/Linux development host, although most testing takes place on Ubuntu "LTS" distributions. The following development packages must be installed on the host. Note these are names of Debian/Ubuntu packages; other Linux distributions should provide a similar set but the package names may vary.

- build-essential (contains gcc, libc-dev, make, etc.)
- cmake (at least v2.8.12 recommended)
- libexpat1-dev
- liblua5.3-dev (older versions of Lua may work, at least v5.1 is required)
- libjson-c-dev (optional; for JSON bindings)
- python3-dev (optional; for python bindings)

## Initial Setup

    git clone https://github.com/OpenSatKit/Runtime-Env.git

## Building the Software

    make SIMULATION=native prep
    make
    make install

## Executing the Software

