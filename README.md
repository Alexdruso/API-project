# API-project
This project was made as homework for an algorithm course held at Politecnico di Milano between 2018 and 2019.


It's inteded porpouse is to simulate in an efficient way the interactions and relationships between entities initialized via the command addent "<ent_name>".

Other command available are:

-addrel "<origin_ent>" "<destination_ent>" "<rel_name>", which adds a relationship named <rel_name> between the two entities; the command is ignored if one of the two entities doens't exist;

-delrel "<origin_ent>" "<destination_ent>" "<rel_name>", which deletes the trlationship named <rel_name> between the two entities; the command is ignored id one of the two entities or the relationship between the two doesn't exist;

-delent "<ent_name>", which deletes <ent_name> from the monitored entities and also deletes all the relationships in which it is involved; the command is ignored if the entity doesn't exist;

-report, whih prints on screen iteratively the name of the type of relationship, the entities with the highest incoming relationships of that type and the number of the maximum incoming relationships; if there are no relationships of that type between the entities, the relationship isn't printed, if there is no relationship between the entities, report prints none;

-end, which terminates the program.
