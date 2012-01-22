@echo off

setlocal

set PATH=%PATH%;%CD%\tools

set WGET_ARGS=--no-check-certificate
set UNZIP_ARGS=-q

set BASEURL=https://s3.amazonaws.com/rstudio-buildtools/
set BOOST_FILE=boost-1.44-win.zip
set MINGW_FILE=mingw64-2010-10-03.zip
set GIN_FILE=gin-1.5.zip
set GWT_FILE=gwt-2.4.0.zip
set JUNIT_FILE=junit-4.9b3.jar
set GNUDIFF_FILE=gnudiff.zip
set MSYS_SSH_FILE=msys_ssh.zip

if not exist boost-win (
  wget %WGET_ARGS% "%BASEURL%%BOOST_FILE%"
  echo Unzipping %BOOST_FILE%
  unzip %UNZIP_ARGS% "%BOOST_FILE%"
  del "%BOOST_FILE%"
)

if not exist mingw64 (
  wget %WGET_ARGS% "%BASEURL%%MINGW_FILE%"
  echo Unzipping %MINGW_FILE%
  unzip %UNZIP_ARGS% "%MINGW_FILE%"
  del "%MINGW_FILE%"
)

if not exist gnudiff (
  wget %WGET_ARGS% "%BASEURL%%GNUDIFF_FILE%"
  mkdir gnudiff
  echo Unzipping %GNUDIFF_FILE%
  unzip %UNZIP_ARGS% "%GNUDIFF_FILE%" -d gnudiff
  del "%GNUDIFF_FILE%"
)

if not exist msys_ssh (
  wget %WGET_ARGS% "%BASEURL%%MSYS_SSH_FILE%"
  mkdir msys_ssh
  echo Unzipping %MSYS_SSH_FILE%
  unzip %UNZIP_ARGS% "%MSYS_SSH_FILE%" -d msys_ssh
  del "%MSYS_SSH_FILE%"
)

mkdir ..\..\src\gwt\lib
pushd ..\..\src\gwt\lib

if not exist gin\1.5 (
  wget %WGET_ARGS% "%BASEURL%%GIN_FILE%"
  mkdir gin\1.5
  echo Unzipping %GIN_FILE%
  unzip %UNZIP_ARGS% "%GIN_FILE%" -d gin\1.5
  del "%GIN_FILE%"
)

if not exist gwt\2.4.0 (
  wget %WGET_ARGS% "%BASEURL%%GWT_FILE%"
  echo Unzipping %GWT_FILE%
  unzip %UNZIP_ARGS% "%GWT_FILE%"
  mkdir gwt
  move gwt-2.4.0 gwt\2.4.0
  del "%GWT_FILE%"
)

if not exist %JUNIT_FILE% (
  wget %WGET_ARGS% "%BASEURL%%JUNIT_FILE%"
)