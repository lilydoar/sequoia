#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

#define DECLARE_STATE_TYPE(name, data_fields)                                  \
  typedef struct name {                                                        \
    data_fields                                                                \
  } name;                                                                      \
  typedef struct name##_State {                                                \
    name data;                                                                 \
    struct name##_Transition *transitions;                                     \
    size_t transition_count;                                                   \
    const char *name;                                                          \
  } name##_State;

#define DECLARE_TRANSITION_TYPE(state_type)                                    \
  typedef struct state_type##_Transition {                                     \
    struct state_type##_State *target;                                         \
    double probability;                                                        \
  } state_type##_Transition;

typedef struct StateTag {
  const char *tag;
  struct StateTag *next;
} StateTag;

typedef struct State {
  void *data;
  size_t data_size;
  struct Transition *transitions;
  size_t transition_count;
  const char *name;
  StateTag *tags;
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

/* StateMatchFunc is used to compare state data with external data.
   Returns non-zero if the state matches the external data.
   state_data: Pointer to the state's internal data
   external_data: Pointer to the data being matched against */
typedef int (*StateMatchFunc)(const void *state_data,
                              const void *external_data);

typedef void json_object;

/* JsonSerializeFunc converts state data to a json_object.
   Returns NULL on error. */
typedef json_object *(*JsonSerializeFunc)(const void *state_data);

/* JsonDeserializeFunc constructs state data from a json_object.
   Returns 0 on success, non-zero on error. */
typedef int (*JsonDeserializeFunc)(void *state_data, json_object *obj);

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

static MarkovError validate_probability(double prob) {
  if (prob < 0.0 || prob > 1.0)
    return MARKOV_INVALID_PROBABILITY;
  return MARKOV_OK;
}

State *add_state(MarkovModel *model, const void *data, size_t data_size,
                 const char *name) {
  if (!model || !data || !name)
    return NULL;

  State *new_states =
      realloc(model->states, sizeof(State) * (model->state_count + 1));
  if (!new_states)
    return NULL;

  model->states = new_states;
  State *state = &model->states[model->state_count];

  state->data = malloc(data_size);
  if (!state->data)
    return NULL;

  memcpy(state->data, data, data_size);
  state->data_size = data_size;
  state->transitions = NULL;
  state->transition_count = 0;
  state->name = strdup(name);
  state->tags = NULL; // Initialize tags as NULL

  if (!state->name) {
    free(state->data);
    return NULL;
  }

  model->state_count++;
  return state;
}

MarkovError add_tag(State *state, const char *tag) {
  if (!state || !tag)
    return MARKOV_INVALID_STATE;

  StateTag *new_tag = malloc(sizeof(StateTag));
  if (!new_tag)
    return MARKOV_MEMORY_ERROR;

  new_tag->tag = strdup(tag);
  if (!new_tag->tag) {
    free(new_tag);
    return MARKOV_MEMORY_ERROR;
  }

  new_tag->next = state->tags;
  state->tags = new_tag;
  return MARKOV_OK;
}

MarkovError remove_tag(State *state, const char *tag) {
  if (!state || !tag)
    return MARKOV_INVALID_STATE;

  StateTag **current = &state->tags;
  while (*current) {
    if (strcmp((*current)->tag, tag) == 0) {
      StateTag *to_remove = *current;
      *current = to_remove->next;
      free((void *)to_remove->tag);
      free(to_remove);
      return MARKOV_OK;
    }
    current = &(*current)->next;
  }
  return MARKOV_OK;
}

bool has_tag(const State *state, const char *tag) {
  if (!state || !tag)
    return false;

  for (StateTag *current = state->tags; current; current = current->next) {
    if (strcmp(current->tag, tag) == 0)
      return true;
  }
  return false;
}

typedef bool (*TagFilter)(const char *tag, void *context);

State **find_states_by_tag(const MarkovModel *model, TagFilter filter,
                           void *context, size_t *count) {
  if (!model || !filter || !count)
    return NULL;

  State **result = NULL;
  *count = 0;

  for (size_t i = 0; i < model->state_count; i++) {
    for (StateTag *tag = model->states[i].tags; tag; tag = tag->next) {
      if (filter(tag->tag, context)) {
        State **new_result = realloc(result, (*count + 1) * sizeof(State *));
        if (!new_result) {
          free(result);
          return NULL;
        }
        result = new_result;
        result[*count] = &model->states[i];
        (*count)++;
        break; // State matched, move to next state
      }
    }
  }

  return result;
}

/* Adds a transition between two states with the given probability
 * from: Source state for the transition
 * to: Target state for the transition
 * probability: Transition probability (must be between 0 and 1)
 * Returns: MARKOV_OK on success, error code otherwise
 *
 * Note: The sum of all transition probabilities from a state
 * should not exceed 1.0
 */
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

/* Determines the next state based on transition probabilities
 * model: The Markov model
 * current: Current state
 * Returns: Next state based on random selection weighted by transition
 * probabilities
 *
 * If no transition is selected (e.g., if random number exceeds all
 * probabilities), returns the current state. This handles cases where
 * transition probabilities sum to less than 1.0
 */
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

/* Finds a state in the model where the state's data matches external_data
 * according to match_func model: The Markov model to search external_data: Data
 * to match against state data match_func: Function that compares state data
 * with external data Returns: Matching state or NULL if no match found
 */
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

/* Serializes a model to a JSON object
 * model: The model to serialize
 * serialize_func: Function to serialize individual state data
 * Returns: JSON object containing serialized model (caller must free with
 * json_object_put)
 *
 * The serialized format includes:
 * - All states with their data and transitions
 * - All tags associated with states
 * - Current state information
 * - Transition probabilities
 */
json_object *serialize_model(const MarkovModel *model,
                             JsonSerializeFunc serialize_func) {
  if (!model || !serialize_func)
    return NULL;

  json_object *root = json_object_new_object();
  if (!root)
    return NULL;

  json_object *states_array = json_object_new_array();
  if (!states_array) {
    json_object_put(root);
    return NULL;
  }
  json_object_object_add(root, "states", states_array);

  for (size_t i = 0; i < model->state_count; i++) {
    json_object *state_obj = json_object_new_object();
    if (!state_obj)
      continue;

    json_object_object_add(state_obj, "name",
                           json_object_new_string(model->states[i].name));

    // Serialize tags if present
    if (model->states[i].tags) {
      json_object *tags_array = json_object_new_array();
      if (tags_array) {
        for (StateTag *tag = model->states[i].tags; tag; tag = tag->next) {
          json_object_array_add(tags_array, json_object_new_string(tag->tag));
        }
        json_object_object_add(state_obj, "tags", tags_array);
      }
    }

    json_object *state_data = serialize_func(model->states[i].data);
    if (state_data) {
      json_object_object_add(state_obj, "data", state_data);
    }

    json_object *transitions = json_object_new_array();
    if (transitions) {
      for (size_t j = 0; j < model->states[i].transition_count; j++) {
        json_object *trans_obj = json_object_new_object();
        if (!trans_obj)
          continue;

        json_object_object_add(
            trans_obj, "target",
            json_object_new_string(
                model->states[i].transitions[j].target->name));
        json_object_object_add(
            trans_obj, "probability",
            json_object_new_double(
                model->states[i].transitions[j].probability));

        json_object_array_add(transitions, trans_obj);
      }
      json_object_object_add(state_obj, "transitions", transitions);
    }

    json_object_array_add(states_array, state_obj);
  }

  if (model->current_state) {
    json_object_object_add(root, "current_state",
                           json_object_new_string(model->current_state->name));
  }

  return root;
}

/* Deserializes a model from a JSON object
 * model: Model to populate with deserialized data
 * root: JSON object containing serialized model
 * deserialize_func: Function to deserialize individual state data
 * Returns: MARKOV_OK on success, error code otherwise
 *
 * JSON format:
 * {
 *   "states": [
 *     {
 *       "name": "state_name",
 *       "data": {...},
 *       "tags": ["tag1", "tag2"],
 *       "transitions": [
 *         {
 *           "target": "target_state_name",
 *           "probability": 0.5
 *         }
 *       ]
 *     }
 *   ],
 *   "current_state": "current_state_name"
 * }
 */
MarkovError deserialize_model(MarkovModel *model, json_object *root,
                              JsonDeserializeFunc deserialize_func) {
  if (!model || !root || !deserialize_func)
    return MARKOV_INVALID_STATE;

  json_object *states_array;
  if (!json_object_object_get_ex(root, "states", &states_array)) {
    return MARKOV_SERIALIZATION_ERROR;
  }

  size_t num_states = json_object_array_length(states_array);
  for (size_t i = 0; i < num_states; i++) {
    json_object *state_obj = json_object_array_get_idx(states_array, i);
    json_object *name_obj;
    json_object *data_obj;

    if (!json_object_object_get_ex(state_obj, "name", &name_obj) ||
        !json_object_object_get_ex(state_obj, "data", &data_obj)) {
      continue;
    }

    void *state_data = malloc(1024); // Assume max state size for simplicity
    if (!state_data)
      return MARKOV_MEMORY_ERROR;

    if (deserialize_func(state_data, data_obj) != 0) {
      free(state_data);
      continue;
    }

    const char *name = json_object_get_string(name_obj);
    State *new_state = add_state(model, state_data, 1024, name);

    // Deserialize tags if present
    json_object *tags_array;
    if (json_object_object_get_ex(state_obj, "tags", &tags_array)) {
      size_t num_tags = json_object_array_length(tags_array);
      for (size_t j = 0; j < num_tags; j++) {
        json_object *tag_obj = json_object_array_get_idx(tags_array, j);
        const char *tag = json_object_get_string(tag_obj);
        if (tag) {
          add_tag(new_state, tag);
        }
      }
    }
    free(state_data);
  }

  // Second pass to set up transitions
  // Second pass: Process transitions for each state
  // This needs to be done after all states are created so we can link
  // transitions
  for (size_t i = 0; i < num_states; i++) {
    json_object *state_obj = json_object_array_get_idx(states_array, i);

    // Get the transitions array and name for the current state
    json_object *transitions_array;
    json_object *name_obj;

    // Skip states without transitions or names
    if (!json_object_object_get_ex(state_obj, "transitions",
                                   &transitions_array)) {
      continue;
    }
    if (!json_object_object_get_ex(state_obj, "name", &name_obj)) {
      continue;
    }

    State *from_state = NULL;
    const char *from_name = json_object_get_string(name_obj);

    for (size_t k = 0; k < model->state_count; k++) {
      if (strcmp(model->states[k].name, from_name) == 0) {
        from_state = &model->states[k];
        break;
      }
    }

    if (!from_state)
      continue;

    size_t num_transitions = json_object_array_length(transitions_array);
    for (size_t j = 0; j < num_transitions; j++) {
      json_object *trans_obj = json_object_array_get_idx(transitions_array, j);
      json_object *target_obj;
      json_object *prob_obj;

      if (!json_object_object_get_ex(trans_obj, "target", &target_obj) ||
          !json_object_object_get_ex(trans_obj, "probability", &prob_obj)) {
        continue;
      }

      const char *target_name = json_object_get_string(target_obj);
      State *target_state = NULL;

      for (size_t k = 0; k < model->state_count; k++) {
        if (strcmp(model->states[k].name, target_name) == 0) {
          target_state = &model->states[k];
          break;
        }
      }

      if (target_state) {
        add_transition(from_state, target_state,
                       json_object_get_double(prob_obj));
      }
    }
  }

  json_object *current_state_obj;
  if (json_object_object_get_ex(root, "current_state", &current_state_obj)) {
    const char *current_name = json_object_get_string(current_state_obj);
    for (size_t i = 0; i < model->state_count; i++) {
      if (strcmp(model->states[i].name, current_name) == 0) {
        model->current_state = &model->states[i];
        break;
      }
    }
  }

  return MARKOV_OK;
}

static void free_tags(StateTag *tags) {
  while (tags) {
    StateTag *next = tags->next;
    free((void *)tags->tag);
    free(tags);
    tags = next;
  }
}

void destroy_model(MarkovModel *model) {
  if (!model)
    return;

  for (size_t i = 0; i < model->state_count; i++) {
    free_tags(model->states[i].tags);
    free(model->states[i].data);
    free(model->states[i].transitions);
    free((void *)model->states[i].name);
  }
  free(model->states);
  free(model);
}
