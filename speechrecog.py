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
        r.adjust_for_ambient_noise(source)
        try:
            audio = r.listen(source, timeout=5, phrase_time_limit=5)
        except:
            return 5

    try:
        word = r.recognize_sphinx(audio).lower().strip()

        print("We heard you say: '" + word + "'")
        if "right" in word:
            return 0
        if "up" in word:
            return 1
        if "left" in word:
            return 2
        if "down" in word:
            return 3
        return 4

    except sr.UnknownValueError:
        print("Whisper could not understand audio")
        return 5
    except sr.RequestError as e:
        print(f"Could not request results from Whisper; {e}")
        return 5

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
        r.adjust_for_ambient_noise(source)
        print("Would you like to keep this map or skip? ('start the game' or 'next')")
        try:
            audio = r.listen(source, timeout=5, phrase_time_limit=5)
        except:
            return 3

    try:
        word = r.recognize_sphinx(audio).lower().strip()
        print("We heard you say: '" + word + "'")
        if "next" in word:
            return 1
        if "start" in word or "game" in word:
            return 2
        return 3

    except sr.UnknownValueError:
        print("Whisper could not understand audio")
        return 3
    except sr.RequestError as e:
        print(f"Could not request results from Whisper; {e}")
        return 3
