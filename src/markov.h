#ifndef MARKOV_H
#define MARKOV_H

#include <stdbool.h>
#include <stddef.h>

// Forward declare json_object to avoid requiring json.h in header
typedef struct json_object json_object;

/* Create a custom state type with the given name and data fields */
#define DECLARE_STATE_TYPE(name, data_fields)                                  \
  typedef struct name {                                                        \
    data_fields                                                                \
  } name;                                                                      \
  typedef struct name##_State {                                                \
    name data;                                                                 \
    struct Transition *transitions;                                            \
    size_t transition_count;                                                   \
    char *name;                                                                \
  } name##_State;

/* Create a transition type for the given state type */
#define DECLARE_TRANSITION_TYPE(state_type)                                    \
  typedef struct state_type##_Transition {                                     \
    state_type##_State *target;                                                \
    double probability;                                                        \
  } state_type##_Transition;

typedef struct StateTag StateTag;
typedef struct State State;
typedef struct Transition Transition;
typedef struct MarkovModel Model;

typedef enum {
  MARKOV_OK = 0,
  MARKOV_INVALID_PROBABILITY = -1,
  MARKOV_INVALID_STATE = -2,
  MARKOV_MEMORY_ERROR = -3,
  MARKOV_SERIALIZATION_ERROR = -4
} MarkovError;

typedef int (*StateMatchFunc)(const void *state_data, const void *external_data);
typedef json_object *(*JsonSerializeFunc)(const void *state_data);
typedef int (*JsonDeserializeFunc)(void *state_data, json_object *obj);
typedef bool (*TagFilter)(const char *tag, void *context);

// Core API
Model *create_model(void);
void destroy_model(Model *model);
State *add_state(Model *model, const void *data, size_t data_size, const char *name);
void set_initial_state(Model *model, size_t idx);
MarkovError move_to_state(Model *model, State *state);
MarkovError step_model(Model *model);

// Transition API
MarkovError add_transition(State *from, State *to, double probability);
void set_transition_callback(Model *model, void (*callback)(const State *from, const State *to, void *context), void *context);

// Tag API
MarkovError add_tag(State *state, const char *tag);
MarkovError remove_tag(State *state, const char *tag);
bool has_tag(const State *state, const char *tag);
State **find_states_by_tag(const Model *model, TagFilter filter, void *context, size_t *count);

// Search API
State *find_matching_state(const Model *model, const void *external_data, StateMatchFunc match_func);

// Serialization API
json_object *serialize_model(const Model *model, JsonSerializeFunc serialize_func);
MarkovError deserialize_model(Model *model, json_object *root, JsonDeserializeFunc deserialize_func);

#endif // MARKOV_H
