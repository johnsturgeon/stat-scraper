# Rocket League BakkesMod Stat Scraper Plugin

Run your own 'tracker' for Rocket League!  Track all your stats and display them on your own web site.

This project contains two separate components:
1. A plugin for [BakkesMod](https://github.com/bakkesmodorg) using the BakkesModSDK
2. A Web Server - A docker container that runs a web server and mongo database that the plugin 
   talks to

## BakkesMod Plugin "Stat Scraper"

### Description
This plugin will capture game data for 2v2 (more to come later) and 
send the data to your own database for long term tracking

### Installation
TODO: write installation instructions for the plugin

## Docker Web Server and Stats Database
There are a few components to the web server that are all in the docker container

* TODO: Link to Docker Hub image (base docker image)
* MongoDB for storing game data (installed via `docker-compose.yaml`)
* Poetry for python virtual environment

### Description

The Stat Web Server has two functions:

* Provide a REST api for the Plugin to send data
* Provide a Web Frontend for viewing in-game information and stats

### Installation

The web server
Requirements: docker compose
1. see [docker-compose.yaml](web_docker/docker-compose.yaml) file for an example 