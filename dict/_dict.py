#!/usr/bin/env python3
# -*- coding:utf-8 -*-

from dictconf import appKey, secretKey, localDir

from urllib.parse import quote
import urllib.request
import hashlib
import random
import json
import os, sys

def show_word(res):

    print(" %s:" %res['query'])
    print("")
    print("    Basic:  %s" %res["translation"][0])
    
    if "us-phonetic" in res["basic"]:
        print("    US:     [%s]" %res["basic"]["us-phonetic"])
    if "uk-phonetic" in res["basic"]:
        print("    UK:     [%s]" %res["basic"]["uk-phonetic"])
    
    print("")
    print("    Explains:  ", end="") 
    i = 1
    for exp in res["basic"]["explains"]:
        if i > 1:
            print("               ", end="")
        i += 1
        p = exp.find(" ")
        if p == -1:
            print(exp)
        else:
            print("%-5s%s"%(exp[:p],exp[p:]))

    print("")
    if "web" in res.keys():
        print("    Web:       ", end="")
        i = 1
        for exp in res["web"]:
            if i > 1:
                print("               ", end="")
            i += 1
            s = ''
            j = 1
            for t in exp["value"]:
                s = s + ("" if j == 1 else ", ") + t
                j += 1 
            print("%s  %s"%(exp["key"],s))


def download_sound(word, speech):
    url = word["basic"][speech]
    word = word['query']
    try:
        response = urllib.request.urlopen(url)
        res = response.read()
    except Exception as e:
        print("can't download sound for %s, %s" % (word, e))
    else:
        filename = os.path.join(localDir , word + ".mp3")
        with open(filename,'wb') as f:
            f.write(res)


def play_sound(word):
    word = word['query']
    try: 
        from playsound import playsound
    except ImportError:
        print("can't import playsound module")
    
    filename = os.path.join(localDir , word + ".mp3")
    if not os.path.isfile(filename):
        print("no sound file for %s" % filename)
        return
    
    playsound(filename)


def download_word(word):
    
    myurl = 'http://openapi.youdao.com/api'
    fromLang = 'EN'
    toLang = 'zh-CHS'
    salt = random.randint(1, 65536)   
    sign = appKey + word + str(salt) + secretKey

    m = hashlib.md5()
    m.update(sign.encode())
    sign = m.hexdigest()

    myurl = myurl + '?appKey=' + appKey + '&q=' + quote(word)  \
            + '&from=' + fromLang + '&to=' + toLang + '&salt=' \
            + str(salt) + '&sign=' + sign

    try:
        response = urllib.request.urlopen(myurl,timeout = 5)
        res =  json.loads(response.read().decode('utf-8'))
        
        if res['errorCode'] != "0":
            print("API Error, code %s", res['errorCode'])
            return None
    
        if not "basic" in res:
            print("Unknow %s"%res['query'])
            return None
        return res
    except Exception as e:
        print (e)
        return None
