# AGL Speech interface draft

This is a draft interface proposal for the low-level [Automotive Grade Linux](https://www.automotivelinux.org/) speech interface that is currently being discussed in the speech expert group. 
The interface encapsulates proprietary speech interfaces and contains both speech input (speech recognition, natural language understanding (NLU)) as well as speech output for multiple languages. 
The speech output contains an interface to play a "prompt", i.e. an arbitrary string to be synthesized into audio. It can optionally contain SSML markup to control the speech synthesis (e.g. volume, rate, embedded audio files, ...). The engine sends events when the prompt playback starts and when it finishes.
The speech input is extremely simplified in this version and is reduced to the event that is raised when an "intent" was recognized. Intents are similar to commands and can be routed to the appropriate AGL application by a higher layer. The current interface proposal does not comprise specification of intents via grammars or NLU models.

This project contains a mock implementation of the speech interface, e.g. when you play a prompt, it raises the events with a certain delay, and when you start the speech recognition, it will send an event with an example phrase after a few seconds. There's no actual interaction with a TTS or speech recognition engine.

# How to build

To build, you can use the provided [Vagrant](https://www.vagrantup.com/) file. Alternatively, you can use any machine with Ubuntu 16.04 and execute the shell commands in Vagrantfile.

Create the VM with
```
vagrant up
```

Then log in with
```
vagrant ssh
```
Inside the VM, run the following commands to build and run the service:
```
cd /vagrant
./conf.d/autobuild/linux/autobuild build
afb-daemon --verbose --ldpaths=build/agl-speech-afb  --port 1235 --token mytoken
```

In another window, you can connect to the service with 
```
afb-client-demo -H ws://localhost:1235/api?token=mytoken
```

Type
`agl-speech subscribe`
to subscribe to events, and then
`agl-speech tts_play_prompt {"language":"en-US","text":"Hello AGL! What can I do for you?"}`
to trigger a fake TTS prompt

A list of languages is available at
`agl-speech tts_get_available_languages`

Speech to text works like this (assume the user said "Please set the temperature to 70 degrees"):
`agl-speech stt_recognize`

Overall, the output looks like this:
```
vagrant@ubuntu-xenial:~$ afb-client-demo -H ws://localhost:1235/api?token=mytoken
agl-speech subscribe
ON-REPLY 1:agl-speech/subscribe: OK
{
  "response":{
    "status":"ok"
  },
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "info":"subscribed to all events",
    "uuid":"27fa106c-4053-42d6-a1cb-b4ed3d4faba7"
  }
}
agl-speech tts_play_prompt {"language":"en-US","text":"Hello AGL! What can I do for you?"}
ON-REPLY 2:agl-speech/tts_play_prompt: OK
{
  "response":{
    "status":"ok"
  },
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "info":"tts_play_prompt"
  }
}
ON-EVENT agl-speech/event_tts_prompt_playing:
{
  "event":"agl-speech\/event_tts_prompt_playing",
  "data":{
    "text":"Hello AGL! What can I do for you?",
    "language":"en-US",
    "elapsed_time_us":2500000
  },
  "jtype":"afb-event"
}
ON-EVENT agl-speech/event_tts_prompt_completed:
{
  "event":"agl-speech\/event_tts_prompt_completed",
  "data":{
    "text":"Hello AGL! What can I do for you?",
    "language":"en-US",
    "elapsed_time_ms":3000
  },
  "jtype":"afb-event"
}
agl-speech tts_get_available_languages
ON-REPLY 3:agl-speech/tts_get_available_languages: OK
{
  "response":{
    "languages":[
      "en-US"
    ]
  },
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "info":"tts_get_available_languages"
  }
}
agl-speech stt_recognize
ON-REPLY 4:agl-speech/stt_recognize: OK
{
  "response":{
    "status":"ok"
  },
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "info":"stt_recognize"
  }
}
ON-EVENT agl-speech/event_stt_final_result:
{
  "event":"agl-speech\/event_stt_final_result",
  "data":{
    "time_offset_usec":5000000,
    "result":{
      "confidence":0.990000,
      "domain":"hvac",
      "intent":"set_temperature",
      "slots":[
        {
          "name":"temperature",
          "value":"70"
        }
      ]
    }
  },
  "jtype":"afb-event"
}
```
