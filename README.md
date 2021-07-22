# Introduction
This project was part of a course on theoretical computer science and algorithms I had the opportunity to follow during my bachelor degree.
The aim of the project was to create a program to manipulate a graph by adding and removing nodes and keeping track of the relationships between the entities. The program is then able to identify the most popular entities according to a given relationship type.
The focus was on performances, both in terms of computational time and memory footprint, assessed through an evaluator.

Final mark: 30/30 cum laude

# Commands

- addent "<ent_name>", which initializes an entity in the graph;

- addrel "<origin_ent>" "<destination_ent>" "<rel_name>", which adds a relationship named <rel_name> between the two entities;

- delrel "<origin_ent>" "<destination_ent>" "<rel_name>", which deletes the trlationship named <rel_name> between the two entities;

- delent "<ent_name>", which deletes <ent_name> from the monitored entities and also deletes all the relationships in which it is involved;

- report, which prints on screen iteratively the name of the type of relationship, the entities with the highest incoming relationships of that type and the number of the maximum incoming relationships;

- end, which terminates the program.
 
