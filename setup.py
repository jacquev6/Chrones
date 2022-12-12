# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations

# https://packaging.python.org/en/latest/

import setuptools


version = "0.1.1"

with open("README.md") as f:
    long_description = f.read()
long_description = long_description.replace("integration-tests/readme-example/report.png", f"https://github.com/jacquev6/Chrones/raw/v{version}/integration-tests/readme-example/report.png")

with open("requirements.txt") as f:
    install_requires = f.readlines()

setuptools.setup(
    name="Chrones",
    version=version,
    description="Software development tool to visualize runtime statistics about your program and correlate them with its phases",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/jacquev6/Chrones",
    author="Vincent Jacques",
    author_email="vincent@vincent-jacques.net",
    license="MIT",
    install_requires=install_requires,
    packages=setuptools.find_packages(),
    include_package_data=True,
    entry_points={
        "console_scripts": [
            "chrones = Chrones.command_line_interface:main",
        ],
    },
)
