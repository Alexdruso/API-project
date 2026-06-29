/*inclusion part*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*definition part*/
#define BEGIN -1
#define ADDENT 1
#define DELENT 2
#define ADDREL 3
#define DELREL 4
#define REPORT 5
#define END 0
#define NAME_BUFFER_SIZE 40
#define NOT_FOUND -1
/*data structures definition part*/

/* A monitored node of the graph. Lives in the open-addressing entity table;
   no intrusive next pointer (the table holds entity* slots directly). */
typedef struct entity {
  char *name;
  struct relation *relations; /* relation types this entity participates in */
} entity;

/* A relation type as seen from a single entity. Adjacency is stored as two
   open-addressing hash tables of entity* (power-of-two `*_cap` slots, NULL =
   empty, `*_len` live entries), grown and rehashed geometrically: `targets`
   are the entities this one points to, `sources` the entities pointing back at
   it (the reverse index that lets delent skip the whole-graph scan).
   incoming_count is the authoritative count of edges entering this entity for
   this relation. */
typedef struct relation {
  char *name;
  unsigned int incoming_count; /* number of edges entering this entity */
  struct relation *next;
  struct entity **targets;
  struct entity **sources;
  unsigned int target_len, target_cap;
  unsigned int source_len, source_cap;
} relation;

/* Per relation type, the current leaders: the entities tied for the highest
   incoming_count. */
typedef struct relation_max {
  char *name;
  unsigned int max_count;    /* the highest incoming_count seen */
  unsigned int leader_count; /* how many entities are tied at max_count */
  char **leaders;            /* names of the tied entities, sorted */
  struct relation_max *next;
} relation_max;

// global variables
relation_max *relation_maxes = NULL;
/* Entity hash table: open-addressing table of entity* (power-of-two slots,
   linear probe, name_hash). Global so table_grow's rehash — which reallocates
   and moves the base pointer — is transparent to every call site. The structs
   themselves never move; only the slot array does, and all aliases elsewhere
   hold entity* to the heap structs, so a rehash dangles nothing. */
entity **entity_table = NULL;
unsigned int entity_cap = 0;
unsigned int entity_len = 0;
/*function definition part*/

/*essential functions*/

relation *find_relation(entity *ent, char *relation_name) {
  relation *node = ent->relations;
  int comparison = -1;

  while (node != NULL &&
         ((comparison = strcmp(node->name, relation_name)) < 0)) {
    node = node->next;
  }

  if (comparison == 0)
    return node;

  return NULL;
}

/* Adjacency is an open-addressing hash table of entity* (linear probe), stored
   inline: `cap` is the power-of-two slot count (NULL = empty), `len` the live
   count. FNV-1a over the name distributes the very similar benchmark names
   ("e00042") better than the entity-table hash's chunk-sum. Iteration order is
   irrelevant: report reads relation_max, never targets/sources. */
static inline unsigned int name_hash(const char *s) {
  unsigned int h = 2166136261u;
  while (*s) {
    h ^= (unsigned char)*s++;
    h *= 16777619u;
  }
  return h;
}

/* Allocate/grow the table to new_cap (power of two), rehashing live entries. */
static void table_grow(entity ***arr, unsigned int *cap, unsigned int new_cap) {
  entity **old = *arr;
  unsigned int old_cap = *cap;
  entity **fresh = calloc(new_cap, sizeof(entity *));
  unsigned int mask = new_cap - 1;

  for (unsigned int i = 0; i < old_cap; i++) {
    if (old[i] != NULL) {
      unsigned int j = name_hash(old[i]->name) & mask;
      while (fresh[j] != NULL)
        j = (j + 1) & mask;
      fresh[j] = old[i];
    }
  }
  free(old);
  *arr = fresh;
  *cap = new_cap;
}

/* Insert `e` if its name is absent. Returns 1 if newly added, 0 if present.
   Grows + rehashes at 0.7 load (also handles the initial cap==0 case). */
int sorted_add(entity ***arr, unsigned int *len, unsigned int *cap, entity *e) {
  if ((*len + 1) * 10 >= *cap * 7) {
    unsigned int new_cap = *cap ? *cap * 2 : 8;
    table_grow(arr, cap, new_cap);
  }

  unsigned int mask = *cap - 1;
  unsigned int j = name_hash(e->name) & mask;
  while ((*arr)[j] != NULL) {
    if (strcmp((*arr)[j]->name, e->name) == 0)
      return 0;
    j = (j + 1) & mask;
  }

  (*arr)[j] = e;
  (*len)++;
  return 1;
}

/* Remove the entity whose name == `name`. Returns removed entity* or NULL.
   Backward-shift deletion keeps probe sequences intact (no tombstones); cap is
   passed by value (never shrinks). */
entity *sorted_remove(entity ***arr, unsigned int *len, unsigned int cap,
                      char *name) {
  if (cap == 0 || *arr == NULL)
    return NULL;

  entity **t = *arr;
  unsigned int mask = cap - 1;
  unsigned int i = name_hash(name) & mask;

  while (t[i] != NULL && strcmp(t[i]->name, name) != 0)
    i = (i + 1) & mask;

  if (t[i] == NULL)
    return NULL;

  entity *removed = t[i];
  unsigned int j = i;
  for (;;) {
    t[i] = NULL;
    for (;;) {
      j = (j + 1) & mask;
      if (t[j] == NULL) {
        (*len)--;
        return removed;
      }
      unsigned int k = name_hash(t[j]->name) & mask;
      int leave = (i <= j) ? (i < k && k <= j) : (i < k || k <= j);
      if (!leave)
        break;
    }
    t[i] = t[j];
    i = j;
  }
}

/* Read-only lookup in the global entity table. Must use the exact same hash and
   linear-probe sequence as sorted_add/sorted_remove (terminating on the first
   empty slot — valid because backward-shift deletion leaves no tombstones). */
entity *entity_lookup(char *name) {
  if (entity_cap == 0 || entity_table == NULL)
    return NULL;

  unsigned int mask = entity_cap - 1;
  unsigned int i = name_hash(name) & mask;

  while (entity_table[i] != NULL) {
    if (strcmp(entity_table[i]->name, name) == 0)
      return entity_table[i];
    i = (i + 1) & mask;
  }
  return NULL;
}

/*functions for managing the maxima*/

int binary_search(char **array, char *entity_name, unsigned int length) {
  int start = 0;
  int end = (int)length - 1;
  int mid = 0;
  int comparison;

  while (start <= end) {
    mid = (start + end) >> 1;

    comparison = strcmp(array[mid], entity_name);

    if (comparison < 0) {
      start = mid + 1;
    } else if (comparison > 0) {
      end = mid - 1;
    } else
      return mid;
  }

  return NOT_FOUND;
}

int remove_from_max(relation_max *rel_max, char *entity_name) {

  unsigned int length = rel_max->leader_count;

  char **leaders = rel_max->leaders;

  int index = binary_search(leaders, entity_name, length);

  if (index != NOT_FOUND) {

    length--;
    rel_max->leader_count = length;

    for (int i = index; i < (int)length; i++) {
      leaders[i] = leaders[i + 1];
    }

    if (rel_max->leader_count > 0) {
      rel_max->leaders = realloc(leaders, length * sizeof(char *));
      return 0;
    } else {
      free(leaders);
      rel_max->leaders = NULL;
      rel_max->max_count = 0;
      return 1;
    }
  }

  /*entity not present among the leaders: no maximum removed*/
  return 0;
}

relation_max *find_relation_max(char *relation_name) {
  relation_max *node = relation_maxes;
  int comparison = -1;

  while (node != NULL &&
         ((comparison = strcmp(node->name, relation_name)) < 0)) {
    node = node->next;
  }

  if (comparison == 0)
    return node;

  return NULL;
}

relation_max *find_or_create_relation_max(char *relation_name) {
  relation_max *node = relation_maxes;
  relation_max *new_node;

  if ((node == NULL) || (strcmp(node->name, relation_name) > 0)) {
    node = malloc(sizeof(relation_max));
    node->name = relation_name;
    node->max_count = 0;
    node->leader_count = 0;
    node->leaders = NULL;
    node->next = relation_maxes;
    relation_maxes = node;
    return node;
  } else {

    if (strcmp(node->name, relation_name) == 0) {
      free(relation_name);
      return node;
    }

    while (node->next != NULL &&
           (strcmp(node->next->name, relation_name) < 0)) {
      node = node->next;
    }

    if (node->next != NULL) {
      if (strcmp(node->next->name, relation_name) == 0) {
        free(relation_name);
        return node->next;
      }
    }

    new_node = malloc(sizeof(relation_max));
    new_node->name = relation_name;
    new_node->max_count = 0;
    new_node->leader_count = 0;
    new_node->leaders = NULL;
    new_node->next = node->next;
    node->next = new_node;
    return new_node;
  }
}

int find_insertion_index(char *name, char **array, int start, int end) {
  int mid = 0;
  int comparison;

  while (start < end) {
    mid = (start + end) >> 1;

    comparison = strcmp(array[mid], name);

    if (comparison < 0) {
      start = mid + 1;
    } else if (comparison > 0) {
      end = mid - 1;
    } else
      return mid;
  }

  if (start == end) {
    if (strcmp(array[start], name) > 0) {
      return start;
    } else {
      return start + 1;
    }
  }

  if (start > end)
    return start;
}

void add_to_max(entity *new_leader, relation_max *rel_max) {

  int index = find_insertion_index(new_leader->name, rel_max->leaders, 0,
                                   (int)rel_max->leader_count - 1);

  rel_max->leader_count++;

  rel_max->leaders =
      realloc(rel_max->leaders, rel_max->leader_count * sizeof(char *));

  char **leaders = rel_max->leaders;

  char *to_insert = new_leader->name;

  char *displaced;

  int length = (int)rel_max->leader_count;

  for (; index < length; index++) {
    displaced = leaders[index];
    leaders[index] = to_insert;
    to_insert = displaced;
  }
}

void set_new_max(entity *new_leader, relation_max *rel_max, int new_count) {
  rel_max->leaders = realloc(rel_max->leaders, sizeof(char *));
  rel_max->leaders[0] = new_leader->name;
  rel_max->max_count = new_count;
  rel_max->leader_count = 1;
}

void recompute_max(relation_max *rel_max, char *relation_name) {
  for (unsigned int i = 0; i < entity_cap; i++) {
    entity *ent = entity_table[i];
    if (ent == NULL)
      continue;

    relation *rel = find_relation(ent, relation_name);

    if (rel != NULL && rel->incoming_count > 0) {
      if (rel->incoming_count == rel_max->max_count) {
        add_to_max(ent, rel_max);
      } else if (rel->incoming_count > rel_max->max_count) {
        set_new_max(ent, rel_max, (int)rel->incoming_count);
      }
    }
  }
}

/*functions dedicated to input*/

/* Block-buffered, unlocked stdin reader. Reading one character at a time with
   getchar()/scanf() spends most of its time in libc stream locking; pulling
   input in 64 KiB blocks and handing out bytes ourselves removes that overhead
   while keeping memory flat (a single fixed buffer, not the whole input). */
#define INPUT_BUFFER_SIZE 65536
static unsigned char input_buffer[INPUT_BUFFER_SIZE];
static int input_len = 0;
static int input_pos = 0;

static inline int next_char(void) {
  if (input_pos >= input_len) {
    input_len = (int)fread(input_buffer, 1, INPUT_BUFFER_SIZE, stdin);
    input_pos = 0;
    if (input_len <= 0)
      return -1;
  }
  return input_buffer[input_pos++];
}

int get_command() {
  int c;
  do {
    c = next_char();
  } while (c == ' ' || c == '\n' || c == '\t' || c == '\r');

  if (c == -1)
    return END;

  /* Only the 1st and 4th characters are needed to disambiguate the commands. */
  char command_line[7];
  int i = 0;
  command_line[i++] = (char)c;
  while ((c = next_char()) != -1 && c != ' ' && c != '\n' && c != '\t' &&
         c != '\r') {
    if (i < 6)
      command_line[i++] = (char)c;
  }
  command_line[i] = '\0';

  if (command_line[0] == 'a') {
    if (command_line[3] == 'e')
      return ADDENT;
    else
      return ADDREL;
  } else if (command_line[0] == 'd') {
    if (command_line[3] == 'e')
      return DELENT;
    else
      return DELREL;
  } else if (command_line[0] == 'r')
    return REPORT;
  else
    return END;
}

char *read_name() {
  char buffer[NAME_BUFFER_SIZE];
  int i = 0;

  while ((buffer[i] = (char)next_char()) != '"')
    ;

  i++;

  while ((buffer[i] = (char)next_char()) != '"') {
    i++;
  }

  i++;

  buffer[i] = '\0';

  i++;

  char *name = malloc(i * sizeof(char));

  for (int j = 0; j < i; ++j) {
    name[j] = buffer[j];
  }

  return name;
}

char *read_name_into(char *buffer) {
  int i = 0;

  while ((buffer[i] = (char)next_char()) != '"')
    ;

  i++;

  while ((buffer[i] = (char)next_char()) != '"') {
    i++;
  }

  i++;

  buffer[i] = '\0';

  return buffer;
}

// functions to handle the report

/*report*/

int report() {
  relation_max *node = relation_maxes;

  char **leaders;
  int active_count = 0;

  while (node != NULL) {
    if (node->max_count != 0)
      active_count++;
    node = node->next;
  }

  if (active_count == 0) {
    fputs("none\n", stdout);
    return 0;
  }

  node = relation_maxes;

  while (node != NULL) {
    unsigned int length = node->leader_count;
    if (length != 0) {
      fputs(node->name, stdout);
      fputc(' ', stdout);
      leaders = node->leaders;
      for (unsigned int i = 0; i < length; i++) {
        fputs(leaders[i], stdout);
        fputc(' ', stdout);
      }
      printf("%d", node->max_count);
      fputc(';', stdout);
      active_count--;
      if (active_count != 0)
        fputc(' ', stdout);
    }
    node = node->next;
  }
  fputc('\n', stdout);
  return 0;
}

/*addent*/
int addent(void) {
  char *entity_name = read_name();

  /* sorted_add stores a pre-existing entity*, so construct the node first (with
     its name set, which sorted_add hashes/compares). If the name is already
     present sorted_add stores nothing and owns nothing, so free both the node
     and the just-read name to match the original free-the-duplicate behavior.
   */
  entity *e = malloc(sizeof(entity));
  e->name = entity_name;
  e->relations = NULL;

  if (sorted_add(&entity_table, &entity_len, &entity_cap, e) == 0) {
    free(entity_name);
    free(e);
  }

  return 0;
}

/*delent*/

relation *pop_first_relation(relation **head) {
  relation *first = *head;
  *head = first->next;
  return first;
}

int delent(void) {
  char entity_name[NAME_BUFFER_SIZE];

  read_name_into(entity_name);

  // removed the entity from the open-addressing table
  entity *deleted_ent =
      sorted_remove(&entity_table, &entity_len, entity_cap, entity_name);
  // if the entity was present

  if (deleted_ent != NULL) {

    // Pop each relation off deleted_ent->relations FIRST, so any self-loop
    // cleanup below calls find_relation(deleted_ent, ...) on a list that no
    // longer contains this node -> returns NULL -> the self-edge is skipped and
    // never double-counted. Same invariant the linked-list version relied on.
    while (deleted_ent->relations != NULL) {

      relation *popped_rel = pop_first_relation(&deleted_ent->relations);
      char *relation_name = popped_rel->name;
      relation_max *rel_max = find_relation_max(relation_name);

      // Phase A: drop this entity from its own relation's leaders if it was
      // one.
      if (rel_max != NULL && popped_rel->incoming_count == rel_max->max_count) {
        int became_empty = remove_from_max(rel_max, deleted_ent->name);

        popped_rel->incoming_count = 0;

        if (became_empty == 1) {
          recompute_max(rel_max, relation_name);
        }
      }

      // Phase B: outgoing edges. For each target D, this entity stops pointing
      // at D, so D loses one incoming edge; also pull this entity out of D's
      // sources table to keep the reverse index consistent. The table is
      // sparse, so iterate all cap slots and skip empties.
      for (unsigned int i = 0; i < popped_rel->target_cap; i++) {
        entity *target_ent = popped_rel->targets[i];
        if (target_ent == NULL)
          continue;
        relation *target_rel = find_relation(target_ent, relation_name);

        if (target_rel != NULL) {
          sorted_remove(&target_rel->sources, &target_rel->source_len,
                        target_rel->source_cap, deleted_ent->name);

          target_rel->incoming_count--;

          if (rel_max != NULL &&
              (target_rel->incoming_count + 1) == rel_max->max_count) {
            int became_empty = remove_from_max(rel_max, target_ent->name);

            if (became_empty == 1) {
              recompute_max(rel_max, relation_name);
            }
          }
        }
      }
      free(popped_rel->targets);

      // Phase C: incoming edges. For each source O, O stops pointing at this
      // entity, so remove this entity from O's targets table. No count change
      // (those edges entered the entity being destroyed). This replaces the old
      // whole-graph scan with O(in-degree) work.
      for (unsigned int i = 0; i < popped_rel->source_cap; i++) {
        entity *source_ent = popped_rel->sources[i];
        if (source_ent == NULL)
          continue;
        relation *source_rel = find_relation(source_ent, relation_name);

        if (source_rel != NULL) {
          sorted_remove(&source_rel->targets, &source_rel->target_len,
                        source_rel->target_cap, deleted_ent->name);
        }
      }
      free(popped_rel->sources);

      free(popped_rel);
    }

    free(deleted_ent->name);
    free(deleted_ent);
  }

  return 0;
}

/*addrel*/

relation *insert_relation(relation **head, char *relation_name) {
  relation *node = *head;
  relation *new_node;

  if ((node == NULL) || (strcmp(node->name, relation_name) > 0)) {
    node = malloc(sizeof(relation));
    node->name = relation_name;
    node->incoming_count = 0;
    node->targets = NULL;
    node->sources = NULL;
    node->target_len = node->target_cap = 0;
    node->source_len = node->source_cap = 0;
    node->next = *head;
    *head = node;
    return node;
  } else {

    if (strcmp(node->name, relation_name) == 0) {
      return node;
    }

    while (node->next != NULL &&
           (strcmp(node->next->name, relation_name) < 0)) {
      node = node->next;
    }

    if (node->next != NULL) {
      if (strcmp(node->next->name, relation_name) == 0) {
        return node->next;
      }
    }

    new_node = malloc(sizeof(relation));
    new_node->name = relation_name;
    new_node->incoming_count = 0;
    new_node->targets = NULL;
    new_node->sources = NULL;
    new_node->target_len = new_node->target_cap = 0;
    new_node->source_len = new_node->source_cap = 0;
    new_node->next = node->next;
    node->next = new_node;
    return new_node;
  }
}

relation *increment_incoming(entity *target, char *relation_name,
                             relation_max *rel_max) {
  relation *target_rel;

  target_rel = insert_relation(&target->relations, relation_name);
  target_rel->incoming_count++;

  if (target_rel->incoming_count > rel_max->max_count)
    set_new_max(target, rel_max, (int)target_rel->incoming_count);
  else if (target_rel->incoming_count == rel_max->max_count)
    add_to_max(target, rel_max);

  return target_rel;
}

void add_relation_edge(entity *origin_ent, entity *destination_ent,
                       char *relation_name, relation_max *rel_max) {

  relation *origin_rel = insert_relation(&origin_ent->relations, relation_name);

  int added = sorted_add(&origin_rel->targets, &origin_rel->target_len,
                         &origin_rel->target_cap, destination_ent);

  if (added == 1) {
    /* Edge is genuinely new: bump the destination's incoming count/leaders and
       mirror the edge into its sources list (the reverse index). Sharing the
       one relation node returned by increment_incoming avoids a second walk of
       the destination's relation list. */
    relation *destination_rel =
        increment_incoming(destination_ent, relation_name, rel_max);
    sorted_add(&destination_rel->sources, &destination_rel->source_len,
               &destination_rel->source_cap, origin_ent);
  }
}

int addrel(void) {
  char *origin = read_name();
  char *destination = read_name();
  char *relation_name = read_name();

  relation_max *rel_max = find_or_create_relation_max(relation_name);

  relation_name = rel_max->name;

  entity *origin_ent = entity_lookup(origin);

  if (origin_ent != NULL) {
    entity *destination_ent = entity_lookup(destination);

    if (destination_ent != NULL) {
      add_relation_edge(origin_ent, destination_ent, relation_name, rel_max);
    }
  }

  free(origin);
  free(destination);
  return 0;
}

/*delrel*/

int delrel(void) {
  char origin[NAME_BUFFER_SIZE];
  char destination[NAME_BUFFER_SIZE];
  char relation_name[NAME_BUFFER_SIZE];

  read_name_into(origin);
  read_name_into(destination);
  read_name_into(relation_name);

  entity *origin_ent = entity_lookup(origin);

  if (origin_ent != NULL) {
    relation *origin_rel = find_relation(origin_ent, relation_name);

    if (origin_rel != NULL && origin_rel->targets != NULL) {
      entity *destination_ent =
          sorted_remove(&origin_rel->targets, &origin_rel->target_len,
                        origin_rel->target_cap, destination);

      if (destination_ent != NULL) {
        relation *destination_rel =
            find_relation(destination_ent, relation_name);

        // keep the reverse index consistent: origin no longer points here
        sorted_remove(&destination_rel->sources, &destination_rel->source_len,
                      destination_rel->source_cap, origin);

        destination_rel->incoming_count--;

        relation_max *rel_max = find_relation_max(relation_name);

        if (rel_max != NULL) {

          if ((destination_rel->incoming_count + 1) == rel_max->max_count) {
            int became_empty = remove_from_max(rel_max, destination_ent->name);

            if (became_empty == 1) {
              recompute_max(rel_max, relation_name);
            }
          }
        }
      }
    }
  }

  return 0;
}

/*debug printing helpers*/
void print_entity_refs(entity **table, unsigned int cap) {
  for (unsigned int i = 0; i < cap; i++) {
    if (table[i] != NULL)
      printf("%s\n", table[i]->name);
  }
}

void print_relations(entity *ent) {
  relation *node = ent->relations;

  if (node == NULL) {
    printf("\nnone\n");
    return;
  }
  while (node != NULL) {
    if (node->target_len != 0) {
      printf("\n-relation \"%s\" with %d incoming with\n", node->name,
             node->incoming_count);
      print_entity_refs(node->targets, node->target_cap);
    } else {
      printf("\n-relation %s with %d incoming and none\n", node->name,
             node->incoming_count);
    }
    node = node->next;
  }
}

void print_entity_table(void) {
  for (unsigned int i = 0; i < entity_cap; i++) {
    entity *ent = entity_table[i];
    if (ent == NULL)
      continue;
    printf("Entity %s has the following relations:", ent->name);
    print_relations(ent);
  }
}
void print_maxes() {
  printf("In summary, the relations are:\n");

  relation_max *node = relation_maxes;

  while (node != NULL) {
    printf("-%s\n", node->name);
    printf("with leaders:\n");
    for (int i = 0; i < (int)node->leader_count; i++) {
      printf("%s\n", node->leaders[i]);
    }
    node = node->next;
  }
}

int main() {

  int command = BEGIN;
  /* Initial cap is a power of two comfortably above the benchmark's ~3000
     entities (load ~0.37), so the table does not grow mid-run and the latency
     signal is clean. calloc zeroes -> all slots empty. */
  entity_cap = 8192;
  entity_len = 0;
  entity_table = calloc(entity_cap, sizeof(entity *));

  while (command != END) {
    command = get_command();

    switch (command) {
    case ADDENT:
      addent();
      break;
    case DELENT:
      delent();
      break;
    case ADDREL:
      addrel();
      break;
    case DELREL:
      delrel();
      break;
    case REPORT:
      report();
      break;
    default:
      break;
    }
  }

  exit(0);
}
