#ifndef MARKOV_H
#define MARKOV_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct State {
  void *data;
  size_t data_size;
  struct Transition *transitions;
  size_t transition_count;
  size_t id;
} State;

typedef struct Transition {
  State *target;
  double probability;
} Transition;

typedef struct MarkovModel {
  State *states;
  size_t state_count;
  State *current_state;
  void (*on_transition)(const State *from, const State *to, void *context);
  void *transition_context;
} MarkovModel;

typedef enum {
  MARKOV_OK = 0,
  MARKOV_INVALID_PROBABILITY = -1,
  MARKOV_INVALID_STATE = -2,
  MARKOV_MEMORY_ERROR = -3,
  MARKOV_SERIALIZATION_ERROR = -4
} MarkovError;

typedef int (*StateMatchFunc)(const void *state_data,
                              const void *external_data);
typedef json_object *(*JsonSerializeFunc)(const void *state_data);
typedef int (*JsonDeserializeFunc)(void *state_data, json_object *obj);

// Core API
MarkovModel *create_model(void) {
  MarkovModel *model = malloc(sizeof(MarkovModel));
  if (!model)
    return NULL;

  model->states = NULL;
  model->state_count = 0;
  model->current_state = NULL;
  model->on_transition = NULL;
  model->transition_context = NULL;
  return model;
}

void destroy_model(MarkovModel *model) {
  if (!model)
    return;

  for (size_t i = 0; i < model->state_count; i++) {
    free(model->states[i].data);
    free(model->states[i].transitions);
  }
  free(model->states);
  free(model);
}

State *add_state(MarkovModel *model, size_t id, const void *data,
                 size_t data_size) {
  assert(model);
  assert(data);

  State *new_states =
      realloc(model->states, sizeof(State) * (model->state_count + 1));
  if (!new_states)
    return NULL;

  model->states = new_states;
  State *state = &model->states[model->state_count];

  state->data = malloc(data_size);
  if (!state->data) {
    return NULL;
  }

  memcpy(state->data, data, data_size);
  state->id = id;
  state->data_size = data_size;
  state->transitions = NULL;
  state->transition_count = 0;

  model->state_count += 1;


  return state;
}

void set_initial_state(MarkovModel *model, size_t idx) {
  assert(model);
  assert(idx < model->state_count);
  model->current_state = &model->states[idx];
}

MarkovError move_to_state(MarkovModel *model, State *state) {
  if (!model || !state)
    return MARKOV_INVALID_STATE;

  State *old_state = model->current_state;
  model->current_state = state;

  if (model->on_transition && old_state) {
    model->on_transition(old_state, state, model->transition_context);
  }

  return MARKOV_OK;
}

State *next_state(const MarkovModel *model, const State *current) {
  if (!model || !current)
    return NULL;

  double random = (double)rand() / RAND_MAX;
  double cumulative = 0.0;

  for (size_t i = 0; i < current->transition_count; i++) {
    cumulative += current->transitions[i].probability;
    if (random <= cumulative) {
      State *next = current->transitions[i].target;
      if (model->on_transition) {
        model->on_transition(current, next, model->transition_context);
      }
      return next;
    }
  }
  return (State *)current;
}

MarkovError step_model(MarkovModel *model) {
  if (!model || !model->current_state)
    return MARKOV_INVALID_STATE;
  model->current_state = next_state(model, model->current_state);
  return MARKOV_OK;
}

State *get_current_state(MarkovModel *model) {
  return &model->states[model->current_state->id];
}
size_t get_current_state_id(MarkovModel *model) {
  assert(model);
  assert(model->current_state);
  return model->current_state->id;
}

// Transition API
static MarkovError validate_probability(double prob) {
  if (prob < 0.0 || prob > 1.0)
    return MARKOV_INVALID_PROBABILITY;
  return MARKOV_OK;
}

MarkovError add_transition(State *from, State *to, double probability) {
  if (!from || !to)
    return MARKOV_INVALID_STATE;

  MarkovError err = validate_probability(probability);
  if (err != MARKOV_OK)
    return err;

  Transition *new_trans = realloc(
      from->transitions, sizeof(Transition) * (from->transition_count + 1));
  if (!new_trans)
    return MARKOV_MEMORY_ERROR;

  from->transitions = new_trans;
  Transition *trans = &from->transitions[from->transition_count];

  trans->target = to;
  trans->probability = probability;
  from->transition_count++;

  return MARKOV_OK;
}

void set_transition_callback(MarkovModel *model,
                             void (*callback)(const State *from,
                                              const State *to, void *context),
                             void *context) {
  if (!model)
    return;
  model->on_transition = callback;
  model->transition_context = context;
}

// Search API
State *find_matching_state(const MarkovModel *model, const void *external_data,
                           StateMatchFunc match_func) {
  if (!model || !external_data || !match_func)
    return NULL;

  for (size_t i = 0; i < model->state_count; i++) {
    if (match_func(model->states[i].data, external_data)) {
      return &model->states[i];
    }
  }
  return NULL;
}

// Serialization API
json_object *serialize_model(const MarkovModel *model,
                             JsonSerializeFunc serialize_func);
MarkovError deserialize_model(MarkovModel *model, json_object *root,
                              JsonDeserializeFunc deserialize_func);

#endif // MARKOV_H
