# mymodule.py
import speech_recognition as sr
import certifi
import sys
import ssl
#

ssl._create_default_https_context = ssl._create_unverified_context


def listenDir():
    r = sr.Recognizer()

    with sr.Microphone() as source:
        print("Pick the direction you'd like to move in. (right, up, left, down)")
        audio = r.listen(source, phrase_time_limit=1)

    try:
        word = r.recognize_whisper(audio, language="english").lower().strip()
        print(word)
        if "right" in word:
            return 1
        if "up" in word:
            return 2
        if "left" in word:
            return 3
        if "down" in word:
            return 4
        return 5

    except sr.UnknownValueError:
        print("Whisper could not understand audio")
    except sr.RequestError as e:
        print(f"Could not request results from Whisper; {e}")

    # try:
    #     maybe = r.recognize_sphinx(audio)
    #     print("Sphinx thinks you said " + maybe)
    #     if maybe == "go west":
    #         return 1
    #     return 0
    #
    # except sr.UnknownValueError:
    #     print("Sphinx could not understand audio")
    #
    # except sr.RequestError as e:
    #     print("Sphinx error; {0}".format(e))


def listenChoice():
    r = sr.Recognizer()

    with sr.Microphone() as source:
        print("Would you like to keep this map or skip? ('select' or 'next')")
        audio = r.listen(source, phrase_time_limit=3)

    try:
        word = r.recognize_whisper(audio, language="english").lower().strip()
        print(word)
        if "next" in word:
            return 1
        if "select" in word:
            return 2
        return 3

    except sr.UnknownValueError:
        print("Whisper could not understand audio")
    except sr.RequestError as e:
        print(f"Could not request results from Whisper; {e}")
