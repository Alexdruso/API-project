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
#define ENTITY_TABLE_SIZE 1009
#define TARGET_TABLE_SIZE 109
#define NAME_BUFFER_SIZE 40
#define NOT_FOUND -1
/*data structures definition part*/

/* A monitored node of the graph. */
typedef struct entity {
  char *name;
  struct relation *relations; /* relation types this entity participates in */
  struct entity *next;
} entity;

/* A bucket node referencing one entity (an edge endpoint). */
typedef struct entity_ref {
  struct entity *entity;
  struct entity_ref *next;
} entity_ref;

/* A relation type as seen from a single entity: the entities it points to
   (targets) plus how many entities point back to it (incoming_count). */
typedef struct relation {
  char *name;
  unsigned int incoming_count; /* number of edges entering this entity */
  struct relation *next;
  struct entity_ref **targets; /* hash table of entities this one points to */
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

entity *remove_entity_ref(entity_ref **bucket, char *target_name) {
  entity_ref *node = *(bucket);
  entity_ref *previous = *(bucket);
  entity *target;

  if (node != NULL && strcmp(node->entity->name, target_name) == 0) {
    *bucket = node->next;
    target = node->entity;
    free(node);
    return target;
  }

  while (node != NULL && strcmp(node->entity->name, target_name) < 0) {
    previous = node;
    node = node->next;
  }

  if (node == NULL || strcmp(node->entity->name, target_name) > 0)
    return NULL;

  previous->next = node->next;
  target = node->entity;
  free(node);

  return target;
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

void recompute_max(relation_max *rel_max, entity **entity_table,
                   char *relation_name) {
  entity *ent = NULL;

  for (int i = 0; i < ENTITY_TABLE_SIZE; i++) {
    ent = entity_table[i];
    while (ent != NULL) {
      relation *rel = find_relation(ent, relation_name);

      if (rel != NULL && rel->incoming_count > 0) {
        if (rel->incoming_count == rel_max->max_count) {
          add_to_max(ent, rel_max);
        } else if (rel->incoming_count > rel_max->max_count) {
          set_new_max(ent, rel_max, (int)rel->incoming_count);
        }
      }

      ent = ent->next;
    }
  }
}

/*functions dedicated to input*/
int get_command() {
  char command_line[7];
  int read = scanf("%s", command_line);

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

  return read;
}

char *read_name() {
  char buffer[NAME_BUFFER_SIZE];
  int i = 0;

  while ((buffer[i] = (char)getchar()) != '"')
    ;

  i++;

  while ((buffer[i] = (char)getchar()) != '"') {
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

  while ((buffer[i] = (char)getchar()) != '"')
    ;

  i++;

  while ((buffer[i] = (char)getchar()) != '"') {
    i++;
  }

  i++;

  buffer[i] = '\0';

  return buffer;
}

/*hash function*/

unsigned int hash(const char *name) {
  int i = 0;
  unsigned int value = 0;
  unsigned int chunk = 0;

  while (name[i] != '\0') {
    for (int j = 0; j < sizeof(int) && name[i] != '\0'; j++) {
      chunk = chunk << 8 * sizeof(char);
      chunk = chunk | (unsigned int)name[i];
      i++;
    }
    value = value + chunk;
  }
  return (unsigned int)value;
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
void insert_entity(entity **bucket, char *entity_name) {
  entity *node = *bucket;
  entity *new_node;

  if ((node == NULL) || (strcmp(node->name, entity_name) > 0)) {
    node = malloc(sizeof(entity));
    node->name = entity_name;
    node->relations = NULL;
    node->next = *bucket;
    *bucket = node;
    return;
  } else {

    if (strcmp(node->name, entity_name) == 0) {
      free(entity_name);
      return;
    }

    while (node->next != NULL && (strcmp(node->next->name, entity_name) < 0)) {
      node = node->next;
    }

    if (node->next != NULL) {
      if (strcmp(node->next->name, entity_name) == 0) {
        free(entity_name);
        return;
      }
    }

    new_node = malloc(sizeof(entity));
    new_node->name = entity_name;
    new_node->relations = NULL;
    new_node->next = node->next;
    node->next = new_node;
    return;
  }
}

int addent(entity **entity_table) {
  unsigned int index;
  char *entity_name = read_name();

  index = hash(entity_name) % ENTITY_TABLE_SIZE;
  insert_entity(&entity_table[index], entity_name);

  return 0;
}

/*delent*/

entity *pop_entity(entity **bucket, char *entity_name) {
  entity *node = *(bucket);
  entity *previous = *(bucket);

  if (node != NULL && strcmp(node->name, entity_name) == 0) {
    *bucket = node->next;
    return node;
  }

  while (node != NULL && strcmp(node->name, entity_name) < 0) {
    previous = node;
    node = node->next;
  }

  if (node == NULL || strcmp(node->name, entity_name) > 0)
    return NULL;

  previous->next = node->next;

  return node;
}

relation *pop_first_relation(relation **head) {
  relation *first = *head;
  *head = first->next;
  return first;
}

entity_ref *pop_first_entity_ref(entity_ref **head) {
  entity_ref *first = *head;
  *head = first->next;
  return first;
}

void remove_entity_references(entity **entity_table, char *entity_name,
                              unsigned int index) {
  entity *current_ent;

  for (int i = 0; i < ENTITY_TABLE_SIZE; i++) {
    current_ent = entity_table[i];

    while (current_ent != NULL) {
      relation *current_rel = current_ent->relations;

      while (current_rel != NULL) {
        if (current_rel->targets != NULL)
          remove_entity_ref(&(current_rel->targets[index]), entity_name);

        current_rel = current_rel->next;
      }

      current_ent = current_ent->next;
    }
  }
}

int delent(entity **entity_table) {
  char entity_name[NAME_BUFFER_SIZE];

  read_name_into(entity_name);

  unsigned int entity_index = hash(entity_name) % ENTITY_TABLE_SIZE;

  // removed the entity from the hash table
  entity *deleted_ent = pop_entity(&entity_table[entity_index], entity_name);
  // if the entity was present

  if (deleted_ent != NULL) {

    // Pop directly off deleted_ent->relations so the head stays valid: a
    // self-relation makes the cleanup below call find_relation(deleted_ent,
    // ...), which must not walk a list node we have already freed.
    while (deleted_ent->relations != NULL) {

      relation *popped_rel = pop_first_relation(&deleted_ent->relations);

      if (popped_rel != NULL) {
        char *relation_name = popped_rel->name;
        relation_max *rel_max = find_relation_max(relation_name);

        if (rel_max != NULL) {

          // remove my entity from the leaders if needed
          if (popped_rel->incoming_count == rel_max->max_count) {
            int became_empty = remove_from_max(rel_max, deleted_ent->name);

            popped_rel->incoming_count = 0;

            if (became_empty == 1) {
              recompute_max(rel_max, entity_table, relation_name);
            }
          }

          // now empty the targets table instead
          if (popped_rel->targets != NULL) {
            entity_ref **targets = popped_rel->targets;
            for (int i = 0; i < TARGET_TABLE_SIZE; i++) {

              while (targets[i] != NULL) {

                entity_ref *popped_ref = pop_first_entity_ref(&targets[i]);
                entity *popped_ent = popped_ref->entity;

                free(popped_ref);

                if (popped_ent != NULL) {
                  relation *target_rel =
                      find_relation(popped_ent, relation_name);

                  if (target_rel != NULL) {
                    target_rel->incoming_count--;

                    if ((target_rel->incoming_count + 1) ==
                        rel_max->max_count) {
                      int became_empty =
                          remove_from_max(rel_max, popped_ent->name);

                      if (became_empty == 1) {
                        recompute_max(rel_max, entity_table, relation_name);
                      }
                    }
                  }
                }
              }
            }
            free(targets);
            popped_rel->targets = NULL;
          }
        }
      }

      free(popped_rel);
    }

    // now delete the traces of the entity
    unsigned int index = hash(deleted_ent->name) % TARGET_TABLE_SIZE;

    remove_entity_references(entity_table, deleted_ent->name, index);

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
    new_node->next = node->next;
    node->next = new_node;
    return new_node;
  }
}

void increment_incoming(entity *target, char *relation_name,
                        relation_max *rel_max) {
  relation *target_rel;

  target_rel = insert_relation(&target->relations, relation_name);
  target_rel->incoming_count++;

  if (target_rel->incoming_count > rel_max->max_count)
    set_new_max(target, rel_max, (int)target_rel->incoming_count);
  else if (target_rel->incoming_count == rel_max->max_count)
    add_to_max(target, rel_max);
  else
    return;
}

entity *find_entity(entity *node, char *entity_name) {
  int comparison = -1;

  while (node != NULL && (comparison = strcmp(node->name, entity_name)) < 0) {
    node = node->next;
  }

  if (comparison == 0)
    return node;

  return NULL;
}

int add_entity_ref(entity *target, entity_ref **head) {
  entity_ref *node = *head;
  entity_ref *new_ref;

  if ((node == NULL) || (strcmp(node->entity->name, target->name) > 0)) {
    node = malloc(sizeof(entity_ref));
    node->entity = target;
    node->next = *head;
    *head = node;
    return 1;
  } else {

    if (strcmp(node->entity->name, target->name) == 0) {
      return 0;
    }

    while (node->next != NULL &&
           (strcmp(node->next->entity->name, target->name) < 0)) {
      node = node->next;
    }

    if (node->next != NULL) {
      if (strcmp(node->next->entity->name, target->name) == 0) {
        return 0;
      }
    }

    new_ref = malloc(sizeof(entity_ref));
    new_ref->entity = target;
    new_ref->next = node->next;
    node->next = new_ref;
    return 1;
  }
}

void add_relation_edge(entity *origin_ent, entity *destination_ent,
                       char *relation_name, relation_max *rel_max) {

  relation *origin_rel = insert_relation(&origin_ent->relations, relation_name);

  if (origin_rel->targets == NULL) {
    origin_rel->targets = calloc(TARGET_TABLE_SIZE, sizeof(entity_ref *));
  }

  unsigned int target_index = hash(destination_ent->name) % TARGET_TABLE_SIZE;

  int added =
      add_entity_ref(destination_ent, &origin_rel->targets[target_index]);

  if (added == 1)
    increment_incoming(destination_ent, relation_name, rel_max);
}

int addrel(entity **entity_table) {
  char *origin = read_name();
  char *destination = read_name();
  char *relation_name = read_name();

  relation_max *rel_max = find_or_create_relation_max(relation_name);

  relation_name = rel_max->name;

  unsigned int index = hash(origin) % ENTITY_TABLE_SIZE;

  entity *origin_ent = find_entity(entity_table[index], origin);

  if (origin_ent != NULL) {
    index = hash(destination) % ENTITY_TABLE_SIZE;
    entity *destination_ent = find_entity(entity_table[index], destination);

    if (destination_ent != NULL) {
      add_relation_edge(origin_ent, destination_ent, relation_name, rel_max);
    }
  }

  free(origin);
  free(destination);
  return 0;
}

/*delrel*/

int delrel(entity **entity_table) {
  char origin[NAME_BUFFER_SIZE];
  char destination[NAME_BUFFER_SIZE];
  char relation_name[NAME_BUFFER_SIZE];

  read_name_into(origin);
  read_name_into(destination);
  read_name_into(relation_name);

  unsigned int index = hash(origin) % ENTITY_TABLE_SIZE;

  entity *origin_ent = find_entity(entity_table[index], origin);

  if (origin_ent != NULL) {
    relation *origin_rel = find_relation(origin_ent, relation_name);

    if (origin_rel != NULL && origin_rel->targets != NULL) {
      index = hash(destination) % TARGET_TABLE_SIZE;
      entity *destination_ent =
          remove_entity_ref(&(origin_rel->targets[index]), destination);

      if (destination_ent != NULL) {
        relation *destination_rel =
            find_relation(destination_ent, relation_name);

        destination_rel->incoming_count--;

        relation_max *rel_max = find_relation_max(relation_name);

        if (rel_max != NULL) {

          if ((destination_rel->incoming_count + 1) == rel_max->max_count) {
            int became_empty = remove_from_max(rel_max, destination_ent->name);

            if (became_empty == 1) {
              recompute_max(rel_max, entity_table, relation_name);
            }
          }
        }
      }
    }
  }

  return 0;
}

/*debug printing helpers*/
void print_entity_refs(entity_ref *ref) {
  while (ref != NULL) {
    printf("%s\n", ref->entity->name);
    ref = ref->next;
  }
}

void print_relations(entity *ent) {
  relation *node = ent->relations;
  entity_ref **targets;

  if (node == NULL) {
    printf("\nnone\n");
    return;
  }
  while (node != NULL) {
    targets = node->targets;
    if (targets != NULL) {
      printf("\n-relation \"%s\" with %d incoming with\n", node->name,
             node->incoming_count);
      for (int i = 0; i < TARGET_TABLE_SIZE; i++) {
        print_entity_refs(targets[i]);
      }
    } else {
      printf("\n-relation %s with %d incoming and none\n", node->name,
             node->incoming_count);
    }
    node = node->next;
  }
}

void print_entity(entity *ent) {
  if (ent == NULL)
    return;
  printf("Entity %s has the following relations:", ent->name);
  print_relations(ent);
  print_entity(ent->next);
}
void print_entity_table(entity **entity_table) {
  for (int i = 0; i < ENTITY_TABLE_SIZE; i++) {
    print_entity(entity_table[i]);
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
  entity *entity_table[ENTITY_TABLE_SIZE] = {NULL};

  while (command != END) {
    command = get_command();

    switch (command) {
    case ADDENT:
      addent(entity_table);
      break;
    case DELENT:
      delent(entity_table);
      break;
    case ADDREL:
      addrel(entity_table);
      break;
    case DELREL:
      delrel(entity_table);
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
