// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Engine.h>

DECLARE_LOG_CATEGORY_EXTERN(logNobunanimEditor, Log, All);


/** Time when this is called. */
#define DEBUG_TIME " : " + FString(__TIME__) + " : "
/** Date when this is called. */
#define DEBUG_DATE "[" + FString(__DATE__) + "]" 
/** Folder name where this is called. */
#define DEBUG_TRACE_FOLDER (FString(__FILE__))
/** Current Line number in the code where this is called. */
#define DEBUG_TRACE_LINE FString::FromInt(__LINE__)
/** Class name + Function name where this is called. */
#define DEBUG_TRACE_CLASS_FUNCTION (FString(__FUNCTION__))
/** Class name where this is called. */
#define DEBUG_TRACE_CLASS (FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT(":")))
/** Function name where this is called. */
#define DEBUG_TRACE_FUNCTION (FString(__FUNCTION__).Right(FString(__FUNCTION__).Len() - FString(__FUNCTION__).Find(TEXT("::")) - 2 ))
/** Function signature where this is called. */
#define DEBUG_TRACE_FUNCTION_SIGNATURE (FString(_FUNSIG_))

#define DEBUG_SEMI_TRACE_CALL FString(DEBUG_TRACE_CLASS_FUNCTION + FString("(")  + DEBUG_TRACE_LINE + FString(")"))
#define DEBUG_TRACE_CALL FString(FString("(") +DEBUG_TRACE_FOLDER + FString("):") + DEBUG_TRACE_CLASS_FUNCTION + FString("> at Line ") + DEBUG_TRACE_LINE)

/** Detailed debug screen message.
* MESSAGE a simple string.
* TIME_TO_DISPLAY time to display on screen.
* COLOR see @FColor::... */
#define DEBUG_SCREEN_LOG(MESSAGE, TIME_TO_DISPLAY, COLOR)\
	GEngine->AddOnScreenDebugMessage(-1, TIME_TO_DISPLAY, COLOR, *(DEBUG_SEMI_TRACE_CALL + ": " + MESSAGE));
/** Detailed debug screen message for formatted string.
* FORMAT_MESSAGE is a formatted string (with %s, %d, %f, ...).
* TIME_TO_DISPLAY time to display on screen.
* COLOR see @FColor::... */
#define DEBUG_SCREEN_LOG_FORMAT(FORMAT_MESSAGE, TIME_TO_DISPLAY, COLOR,...)\
	GEngine->AddOnScreenDebugMessage(-1, TIME_TO_DISPLAY, COLOR, *(DEBUG_SEMI_TRACE_CALL + FString(": ") + (FString::Printf(TEXT(FORMAT_MESSAGE), ##__VA_ARGS__)))) ;

/** Detailed debug log message.
* LOG_LEVEL (Log, Warning, Error, Fatal).
* MESSAGE a simple string.*/
#define DEBUG_LOG(LOG_LEVEL, MESSAGE)\
	UE_LOG(logNobunanimEditor, LOG_LEVEL, TEXT("%s:\n\t|=> %s"), *DEBUG_TRACE_CALL, *FString(MESSAGE))
/** Detailed debug log message for formatted string.
* LOG_LEVEL (Log, Warning, Error, Fatal).
* FORMAT_MESSAGE is a formatted string (with %s, %d, %f, ...). */
#define DEBUG_LOG_FORMAT(LOG_LEVEL, FORMAT_MESSAGE, ...)\
	UE_LOG(logNobunanimEditor, LOG_LEVEL, TEXT("%s:\n\t|=> %s"), *DEBUG_TRACE_CALL, *FString::Printf(TEXT(FORMAT_MESSAGE), ##__VA_ARGS__))


/** Detailed debug screen + log message.
* LOG_LEVEL (Log, Warning, Error, Fatal).
* MESSAGE a simple string.
* TIME_TO_DISPLAY time to display on screen.
* COLOR see @FColor::... */
#define DEBUG_LOG_ALL(LOG_LEVEL, MESSAGE, TIME_TO_DISPLAY, COLOR)\
	DEBUG_LOG(LOG_LEVEL, MESSAGE)\
	DEBUG_SCREEN_LOG(MESSAGE, TIME_TO_DISPLAY, COLOR)

/** Detailed debug screen + log message for formatted string.
* LOG_LEVEL (Log, Warning, Error, Fatal).
* FORMAT_MESSAGE is a formatted string (with %s, %d, %f, ...). 
* TIME_TO_DISPLAY time to display on screen.
* COLOR see @FColor::... */
#define DEBUG_LOG_ALL_FORMAT(LOG_LEVEL, FORMAT_MESSAGE, TIME_TO_DISPLAY, COLOR, ...)\
	DEBUG_LOG_FORMAT(LOG_LEVEL, FORMAT_MESSAGE,##__VA_ARGS__)\
	DEBUG_SCREEN_LOG_FORMAT(FORMAT_MESSAGE, TIME_TO_DISPLAY, COLOR,##__VA_ARGS__)
