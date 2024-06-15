# mymodule.py
import speech_recognition as sr
import certifi
import sys
import ssl
#

ssl._create_default_https_context = ssl._create_unverified_context


def listen():
    r = sr.Recognizer()

    with sr.Microphone() as source:
        print("Say something")
        audio = r.listen(source, phrase_time_limit=1)

    try:
        word = r.recognize_whisper(audio, language="english").lower().strip()
        print(word)
        if word == "go right.":
            return 1
        if word == "go up.":
            return 2
        if word == "go left.":
            return 3
        if word == "go down.":
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
