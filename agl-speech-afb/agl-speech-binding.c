#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <systemd/sd-event.h>
#include <sys/timerfd.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

afb_event 	event_tts_prompt_playing, event_tts_prompt_completed, 
			event_stt_result;

static void subscribe(struct afb_req request)
{
	afb_req_subscribe(request, event_tts_prompt_playing);
	afb_req_subscribe(request, event_tts_prompt_completed);
	afb_req_subscribe(request, event_stt_result);

	json_object *res = json_object_new_object();
	json_object_object_add(res, "status", json_object_new_string("ok"));

	afb_req_success(request, res, "subscribed to all events");

	AFB_NOTICE("subscribe was called");
}

static void tts_get_available_languages(struct afb_req request)
{
	json_object *res = json_object_new_object();
	json_object *list = json_object_new_array();
	json_object_array_add(list, json_object_new_string("en-US"));
	json_object_object_add(res, "languages", list);

	afb_req_success(request, res, "tts_get_available_languages");

	AFB_NOTICE("tts_get_available_languages was called");
}

static int on_prompt_completed(
	sd_event_source *s,
 	uint64_t usec,
 	void *userdata)
{
	AFB_NOTICE("TTS prompt completed");

	json_object* event_json = (json_object*) userdata;
	afb_event_push(event_tts_prompt_completed, event_json);

	return 0;
}


// Requests look like this:
// {
//	 "language": "en-US",
//   "text": "Hello AGL! What can I do for you?"
// }
// 
static void tts_play_prompt(struct afb_req request)
{
	json_object *tmpJ;
	json_object *queryJ = afb_req_json(request);

	json_bool success = json_object_object_get_ex(queryJ, "language", &tmpJ);
	if (!success) {
		afb_req_fail_f(request, "ERROR", "key language not found in '%s'", json_object_get_string(queryJ));
		return;
	}

	if (json_object_get_type(tmpJ) != json_type_string) {
		afb_req_fail(request, "ERROR", "key language not a string");
		return;
	}

	const char* language = json_object_get_string(tmpJ);
	if (strncmp(language, "en-US", 4) != 0) {
		afb_req_fail(request, "ERROR", "Only supported language for now is en-US");
		return;
	}


	success = json_object_object_get_ex(queryJ, "text", &tmpJ);
	if (!success) {
		afb_req_fail_f(request, "ERROR", "key text not found in '%s'", json_object_get_string(queryJ));
		return;
	}

	if (json_object_get_type(tmpJ) != json_type_string) {
		afb_req_fail(request, "ERROR", "key text not a string");
		return;
	}

	const char* text = json_object_get_string(tmpJ);
	AFB_NOTICE("Text to speech playing prompt: '%s'", text);

	json_object *res = json_object_new_object();
	json_object_object_add(res, "status", json_object_new_string("ok"));

	// return success
	afb_req_success(request, res, "tts_play_prompt");


	// send event_tts_prompt_playing
	json_object *event_playing_json = json_object_new_object();
	json_object_object_add(event_playing_json, "text", json_object_new_string(text));
	json_object_object_add(event_playing_json, "language", json_object_new_string(language));
	json_object_object_add(event_playing_json, "elapsed_time_us", json_object_new_int(2500000));

	afb_event_push(event_tts_prompt_playing, event_playing_json);


	// create timer that fire event_tts_prompt_completed
	struct sd_event* event_loop = afb_daemon_get_event_loop();

	uint64_t current_time = 0;
	sd_event_now(
		event_loop,
		CLOCK_MONOTONIC,
		&current_time
	);

	json_object *event_completed_json = json_object_new_object();
	json_object_object_add(event_completed_json, "text", json_object_new_string(text));
	json_object_object_add(event_completed_json, "language", json_object_new_string(language));
	json_object_object_add(event_completed_json, "elapsed_time_ms", json_object_new_int(3000));

	sd_event_add_time(
		event_loop,
		NULL, /* event source */
		CLOCK_MONOTONIC,
		current_time + 3000000 , /* usec */
		0, /* accuracy */
		on_prompt_completed,
		event_completed_json /* userdata */
	);
}


struct stt_fake_slot
{
	const char* name;
	const char* value;
};

struct stt_fake_result 
{
	int type; /* 0=intermediate, 1=final */
	int usec_delay;
	double confidence;
	const char* domain;
	const char* intent;
	const struct stt_fake_slot slots; // only one because I'm lazy
};

static const struct stt_fake_result stt_fake_results[] = {
	{	
		.type = 1, 
		.usec_delay = 5000000, 
		.confidence = 0.99,
		.domain = "hvac", 
		.intent = "set_temperature",
		.slots = {
				.name = "temperature",
				.value = "70"
			}
	}
};

static int on_recognition_result(
	sd_event_source *s,
 	uint64_t usec,
 	void *userdata)
{
	struct stt_fake_result result = stt_fake_results[0];
	AFB_NOTICE("Speech to text recognition result at offset %llu with intent '%s'",
		(unsigned long long)result.usec_delay,
		result.intent);

	json_object *event_json = json_object_new_object();

	json_object *intent_result = json_object_new_object();
	json_object_object_add(intent_result, "confidence", json_object_new_double(result.confidence));
	json_object_object_add(intent_result, "domain", json_object_new_string(result.domain));
	json_object_object_add(intent_result, "intent", json_object_new_string(result.intent));

	json_object_object_add(event_json, "time_offset_usec", json_object_new_int(result.usec_delay));
	json_object_object_add(event_json, "result", intent_result);

	json_object *slots = json_object_new_array();
	json_object *slot = json_object_new_object();
	json_object_object_add(slot, "name", json_object_new_string(result.slots.name));
	json_object_object_add(slot, "value", json_object_new_string(result.slots.value));
	json_object_array_add(slots, slot);

	json_object_object_add(intent_result, "slots", slots);

	afb_event_push(event_stt_result, event_json);

	return 0;
}

// Configuration comes later...
static void stt_recognize(struct afb_req request)
{
	json_object *res = json_object_new_object();
	json_object_object_add(res, "status", json_object_new_string("ok"));

	afb_req_success(request, res, "stt_recognize");

	struct sd_event* event_loop = afb_daemon_get_event_loop();
	
	uint64_t current_time = 0;
	sd_event_now(
		event_loop,
		CLOCK_MONOTONIC,
		&current_time
	);

	sd_event_add_time(
		event_loop,
		NULL /* event source */,
		CLOCK_MONOTONIC,
		current_time + stt_fake_results[0].usec_delay , /* usec */
		0, /* accuracy */
		on_recognition_result,
		NULL /* userdata */
	);

	AFB_NOTICE("stt_recognize was called");
}


static const struct afb_auth _afb_auths_v2_monitor[] = {
	{.type = afb_auth_Permission, .text = "urn:AGL:permission:monitor:public:set"},
	{.type = afb_auth_Permission, .text = "urn:AGL:permission:monitor:public:get"},
	{.type = afb_auth_Or, .first = &_afb_auths_v2_monitor[1], .next = &_afb_auths_v2_monitor[0]}
};

static const struct afb_verb_v2 verbs[] = {

	{.verb = "subscribe", .session = AFB_SESSION_NONE, .callback = subscribe, .auth = NULL},

	{.verb = "tts_play_prompt", .session = AFB_SESSION_NONE, .callback = tts_play_prompt, .auth = NULL},
	{.verb = "tts_get_available_languages", .session = AFB_SESSION_NONE, .callback = tts_get_available_languages, .auth = NULL},

	{.verb = "stt_recognize", .session = AFB_SESSION_NONE, .callback = stt_recognize, .auth = NULL},

	{NULL}
};

static int init()
{
	AFB_NOTICE("init was called");
 	event_tts_prompt_playing = afb_daemon_make_event("event_tts_prompt_playing");
	event_tts_prompt_completed = afb_daemon_make_event("event_tts_prompt_completed");
	event_stt_result = afb_daemon_make_event("event_stt_final_result");

    if (!afb_event_is_valid(event_tts_prompt_playing) ||
		!afb_event_is_valid(event_tts_prompt_completed) ||
		!afb_event_is_valid(event_stt_result))
	{
		AFB_ERROR("Failed to create events!");
		return -1;
	}

	return 0;
}

static int preinit()
{
	AFB_NOTICE("preinit was called");
	return 0;
}

static void onevent(const char *event, struct json_object *object)
{
	AFB_NOTICE("onevent called with event %s", event);
}

const struct afb_binding_v2 afbBindingV2 = {
	.api = "agl-speech",
	.specification = NULL,
	.verbs = verbs,
	.preinit = preinit,
	.init = init,
	.onevent = onevent,
	.noconcurrency = 0
};
