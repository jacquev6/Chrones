# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

# https://packaging.python.org/en/latest/

import setuptools


version = '0.0.6'

with open("README.md") as f:
    long_description = f.read()
long_description = long_description.replace("example/example.png", f"https://github.com/jacquev6/Chrones/raw/v{version}/example/example.png")

setuptools.setup(
    name="Chrones",
    version=version,
    description="Software development tool to visualize runtime statistics about your program and correlate them with its phases",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/jacquev6/Chrones",
    author="Vincent Jacques",
    author_email="vincent@vincent-jacuqes.net",
    license="MIT",
    install_requires=open("requirements.txt").readlines(),
    entry_points={
        "console_scripts": [
            "chrones = Chrones.command_line_interface:main",
        ],
    },
)
