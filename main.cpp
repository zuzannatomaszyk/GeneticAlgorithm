#include <iostream>
#include <vector>
#include <utility>
#include <cstdlib>
#include <random>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <ctime>

#define N 100
#define K 20

#define OPERATION_TIME_MEAN 30
#define OPERATION_TIME_SD 15

#define MAINTENANCE_DURATION_MEAN 25
#define MAINTENANCE_DURATION_SD 6

#define POPULATION_SIZE 200
#define RUNNING_TIME 5     //////////////////////DO USTALENIA

using namespace std;

struct operation{
    vector <pair<int, int>> maintenance; // para: start, duration
    vector <int> task_duration_time;
};

struct inst{
    pair <operation, operation> all;
};

bool check_maintenance(vector<pair<int,int>> &first_machine_maintenances, vector<pair<int,int>> &second_machine_maintenances, pair<int,int> &maintenance) {
    bool all_good = true;
    for (auto i : first_machine_maintenances) {
        if (!((i.first + i.second) > maintenance.first || (maintenance.first + maintenance.second) > i.first)) {
            all_good = false;
            break;
        }
    }
    for (auto i : second_machine_maintenances) {
        if (!((i.first + i.second) > maintenance.first || (maintenance.first + maintenance.second) > i.first)) {
            all_good = false;
            break;
        }
    }
    return all_good;
}

inst make_instance(int n, int k) { //n - liczba zadań, k - liczba przerw

    auto firts_operations = new operation;
    auto second_operations = new operation;

    random_device r;
    default_random_engine gen(r());
    normal_distribution<> operation_time_distribution{double(OPERATION_TIME_MEAN),
                                                      double(OPERATION_TIME_SD)}; //{mean, standard deviation}

    int first_task_time = 0;
    int second_task_time = 0;

    for (int i = 0; i < n; i++) {

        do {
            first_task_time = int(operation_time_distribution(gen));
            second_task_time = int(operation_time_distribution(gen));
        } while (first_task_time <= 0 || second_task_time <= 0);

        firts_operations->task_duration_time.push_back(first_task_time);
        second_operations->task_duration_time.push_back(second_task_time);
    }

    pair<int, int> maintenance_start_range;
    maintenance_start_range.first = 1;
    maintenance_start_range.second = n * OPERATION_TIME_MEAN;


    uniform_int_distribution<> maintenance_start_distribution(maintenance_start_range.first,
                                                              maintenance_start_range.second);
    normal_distribution<> maintenance_duration_distribution{double(MAINTENANCE_DURATION_MEAN),
                                                            double(MAINTENANCE_DURATION_SD)};
    pair<int, int> temp_maintenance; //pair: start, duration
    int machine_number = 0;
    int start_time;
    int duration_time;
    for (int i = 0; i < k; i++) {

        do {
            start_time = maintenance_start_distribution(gen);
            duration_time = int(maintenance_duration_distribution(gen));

            temp_maintenance.first = start_time;
            temp_maintenance.second = duration_time;


        } while (duration_time <= 0 || (!check_maintenance(firts_operations->maintenance, second_operations->maintenance, temp_maintenance)));

        if(machine_number == 0){
            firts_operations->maintenance.push_back(temp_maintenance);
        } else if (machine_number == 1){
            second_operations->maintenance.push_back(temp_maintenance);
        }

        if(i == k/2)
            machine_number++;
    }


    auto instance = new inst;
    instance->all.first = *firts_operations;
    instance->all.second = *second_operations;

    return *instance;

}

void save_instances_to_file(vector<inst> &instances, string &file_name){
    ofstream file;
    file.open(file_name);
    for(int i = 1; i < instances.size() + 1; i++){
        file << "**** " << i << " ****" << endl;
        int number_of_tasks = int (instances[i - 1].all.first.task_duration_time.size());
        file << number_of_tasks << endl;
        for (int j = 0; j < number_of_tasks; j++){
            file << instances[i - 1].all.first.task_duration_time[j] << "; " << instances[i - 1].all.second.task_duration_time[j] << endl;
        }
        int maintanance_number = 1;
        for (auto maintanance : instances[i - 1].all.first.maintenance){
            file << maintanance_number << "; 0; "
            << maintanance.second << "; " << maintanance.first << endl;
            maintanance_number++;
        }
    }
    file << "**** EOF ****" << endl;

    file.close();
}

vector <inst> read_instance_from_file(string &file_name){
    int machine_number = 0;
    int maintenance_start = 0;
    int maintenance_duration = 0;

    pair< int,int> maintenance; //first: start time, second: duration time

    auto firts_operations = new operation;
    auto second_operations = new operation;

    inst instance;
    vector<inst> all_instances;

    ifstream file(file_name);
    string line;

    if (!file.good()) {
        fprintf(stderr, "Nie udało się otworzyć pliku!");
        exit(2);
    } else {
        while (getline(file, line)){
            if (line.find("**") == string::npos){
                string number;
                vector<int> numbers_in_line;
                stringstream string_line(line);
                while ( getline(string_line, number, ';')){
                    numbers_in_line.push_back(stoi(number));
                }

                if(numbers_in_line.size() == 2){
                   firts_operations->task_duration_time.push_back(numbers_in_line[0]);
                   second_operations->task_duration_time.push_back(numbers_in_line[1]);
                } else if (numbers_in_line.size() == 4){
                    machine_number = numbers_in_line[1];
                    maintenance_duration = numbers_in_line[2];
                    maintenance_start = numbers_in_line[3];

                    maintenance.first = maintenance_start;
                    maintenance.second = maintenance_duration;
                    if(machine_number == 0){
                        firts_operations->maintenance.push_back(maintenance);
                    } else if(machine_number == 1){
                        second_operations->maintenance.push_back(maintenance);
                    }
                }

            } else {
                instance.all.first = *firts_operations;
                instance.all.second = *second_operations;
                if(!instance.all.first.task_duration_time.empty()) {
                    all_instances.push_back(instance);
                    firts_operations = new operation;
                    second_operations = new operation;
                }
            }
        }
    }
    file.close();

    return all_instances;
}

struct machine{
    //rozwiązanie:
    vector <pair<int, int>> tasks; // para: operation number, finish time
    int finish_time = 0;
};

struct solution{
    pair<machine, machine> task_order;
    int finish_time = 0;
};

machine* first_machine_on(vector<int> shuffled_tasks, inst &operations){
    auto first_machine = new machine;
    int start_time = 0;
    int duration = 0;
    float addition = 1.2;
    bool one_gap = false;
    pair <int, int> temp_task; // para: operation number, finish time
    for(auto i : shuffled_tasks){
        duration = operations.all.first.task_duration_time[i];
        for(auto gap : operations.all.first.maintenance){
            if(gap.first > start_time && gap.first <= start_time + duration){
                if(!one_gap){ //kara naliczana tylko raz, nawet jeżeli zadanie jest przerywane wiele razy
                    duration = int(ceil(duration * addition));
                }
                one_gap = true;
                duration += gap.second;
            } else if(gap.first == start_time){
                duration += gap.second;
            }
        }
        one_gap = false;
        temp_task.first = i;
        temp_task.second = start_time + duration;
        first_machine->tasks.push_back(temp_task);
        start_time += duration;
    }
    first_machine->finish_time = first_machine->tasks.back().second;

    return first_machine;
}

machine* second_machine_on(machine* &first_machine, inst &operations){
    random_device r;
    default_random_engine gen(r());

    auto second_machine = new machine;
    int start_time = first_machine->tasks[0].second;
    int duration = 0;
    float addition = 1.2;
    bool one_gap = false;
    pair <int, int> temp_task; // para: operation number, finish time
    vector <int> finished_on_first_machine;
    vector <pair<int, int>> first_tasks = first_machine->tasks; // zadania z pierewszej maszyny first: numer zadania, second: czas jego zakończenia na pierwszej maszynie

    while (second_machine->tasks.size() != first_machine->tasks.size()){
        int id = 0;
        vector<int> to_erase;
        for (auto i : first_tasks){
            if(i.second <= start_time){
                finished_on_first_machine.push_back(i.first);
                to_erase.push_back(id);
            }
            id++;
        }
        first_tasks.erase(first_tasks.begin(), first_tasks.begin() + to_erase.size());

        if(finished_on_first_machine.empty()){
            start_time = first_tasks.front().second;
        } else {
            uniform_int_distribution<> random_task_id(0, int(finished_on_first_machine.size() - 1));
            int rand_i = random_task_id(gen);int task_nr = finished_on_first_machine[rand_i];
            finished_on_first_machine.erase(finished_on_first_machine.begin() + rand_i);
            duration = operations.all.second.task_duration_time[task_nr];
            for(auto gap : operations.all.second.maintenance){
                if(gap.first > start_time && gap.first <= start_time + duration){
                    if(!one_gap){ //kara naliczana tylko raz, nawet jeżeli zadanie jest przerywane wiele razy
                        duration = int(ceil(duration * addition));
                    }
                    one_gap = true;
                    duration += gap.second;
                } else if(gap.first == start_time){
                    duration += gap.second;
                } else if(gap.first < first_machine->tasks[0].second + 1 && gap.first + gap.second > first_machine->tasks[0].second + 1){
                    duration += gap.first + gap.second - first_machine->tasks[0].second;
                }
            }

            one_gap = false;
            temp_task.first = task_nr;
            temp_task.second = start_time + duration;
            second_machine->tasks.push_back(temp_task);
            start_time += duration;
        }
    }
    second_machine->finish_time = second_machine->tasks.back().second;

    return second_machine;
}

solution* random_solution(inst &operations){
    auto first_machine = new machine;
    auto second_machine = new machine;
    pair<machine, machine> machines;

    vector<int> shuffled_tasks;
    shuffled_tasks.reserve(operations.all.first.task_duration_time.size());
    random_device r;
    default_random_engine gen(r());
    for (int i=0; i < operations.all.first.task_duration_time.size(); i++) shuffled_tasks.push_back(i);
    shuffle(shuffled_tasks.begin(), shuffled_tasks.end(), gen);

    first_machine = first_machine_on(shuffled_tasks, operations);
    second_machine = second_machine_on(first_machine, operations);

    machines.first = *first_machine;
    machines.second = *second_machine;

    auto rand_solution = new solution;

    rand_solution->task_order = machines;
    rand_solution->finish_time = max(machines.first.finish_time, machines.second.finish_time);
    return rand_solution;
}

solution* mutation(solution* &original_solution,  inst &operations){
    random_device r;
    default_random_engine gen(r());

    uniform_int_distribution<> random_id(0, int(original_solution->task_order.first.tasks.size() - 1));
    int first_rand = random_id(gen);
    int second_rand;
    do {
        second_rand = random_id(gen);
    } while (second_rand == first_rand);

    vector <int> shuffled_tasks;
    for (auto i : original_solution->task_order.first.tasks){
        shuffled_tasks.push_back(i.first);
    }
    iter_swap(shuffled_tasks.begin()+first_rand, shuffled_tasks.begin()+second_rand);

    auto first_machine = new machine;
    auto second_machine = new machine;
    first_machine = first_machine_on(shuffled_tasks, operations);

    second_machine = second_machine_on(first_machine, operations);

    pair<machine, machine> machines;
    machines.first = *first_machine;
    machines.second = *second_machine;

    auto after_mutation = new solution;

    after_mutation->task_order = machines;
    after_mutation->finish_time = max(machines.first.finish_time, machines.second.finish_time);

    return after_mutation;


}

solution* crossover(pair<solution, solution> &parents, inst &operations){
    random_device r;
    default_random_engine gen(r());

    uniform_int_distribution<> random_id(0, int(parents.first.task_order.first.tasks.size() - 1));
    int temp_rand;
    vector<int> random_indexes;
    for(int i = 0; i < int(parents.first.task_order.first.tasks.size()/2); i++){
        do {
            temp_rand = random_id(gen);
        } while(find(random_indexes.begin(),random_indexes.end(),temp_rand) != random_indexes.end());
        random_indexes.push_back(temp_rand);
    }
    sort(random_indexes.begin(), random_indexes.end());
    vector<int> values;
    values.reserve(random_indexes.size());
    for(auto i : random_indexes){
        values.push_back(parents.first.task_order.first.tasks[i].first); //pierwszy rodzic -> kolejność zadań -> na pierwszej maszynie -> zadanie o indeksie -> numer zadania
    }

    vector<int> child;
    int id = 0;
    for(auto i : parents.second.task_order.first.tasks){
        if(find(values.begin(), values.end(), i.first) != values.end()) {
            child.push_back(values[id]);
            id++;
        } else {
            child.push_back(i.first);
        }
    }

    auto first_machine = new machine;
    auto second_machine = new machine;
    first_machine = first_machine_on(child, operations);
    second_machine = second_machine_on(first_machine, operations);

    pair<machine, machine> machines;
    machines.first = *first_machine;
    machines.second = *second_machine;

    auto child_solution = new solution;

    child_solution->task_order = machines;
    child_solution->finish_time = max(machines.first.finish_time, machines.second.finish_time);

    return child_solution;
}

bool by_finish_time (solution* i,solution* j) { return (i->finish_time < j->finish_time); }

void genetic_algorithm(inst instance){ // BEDZIE ZWRACAĆ CZAS WYKONANIA SIĘ ZADAŃ DLA TEJ INSTANCJI W PODANYM CZASIE
    clock_t start;
    double duration = 0;
    random_device r;
    default_random_engine gen(r());

    start = clock();
    vector<solution*> population;
    population.reserve(POPULATION_SIZE);
    for(int i = 0; i < POPULATION_SIZE; i++){
        population.push_back(random_solution(instance));
    }
    sort(population.begin(), population.end(), by_finish_time);

    int init_time = population.front()->finish_time;
    cout << "Najlepszy czas dla populacji początkowej: " << init_time << endl;
    while (duration <= RUNNING_TIME){

        vector<solution*>::const_iterator first = population.begin();
        vector<solution*>::const_iterator last = population.begin() + int(POPULATION_SIZE * 0.35);
        vector<solution*> best_of(first, last);

        int temp_rand;
        vector <int> random_partent_id;
        uniform_int_distribution<> random_parent(int(POPULATION_SIZE * 0.35), POPULATION_SIZE - 1);
        for (int i = 0; i < POPULATION_SIZE * 0.15; i++) {
            do {
                temp_rand = random_parent(gen);
            } while(find(random_partent_id.begin(),random_partent_id.end(),temp_rand) != random_partent_id.end());
            random_partent_id.push_back(temp_rand);
        }
        for (auto i : random_partent_id){
            best_of.push_back(population[i]);
        }
        population.clear();
        uniform_int_distribution<> random_id(0, int(POPULATION_SIZE * 0.35) - 1);
        int first_rand;
        int second_rand;
        vector<pair<solution, solution>> random_pairs;
        random_pairs.reserve(POPULATION_SIZE);

        pair<solution, solution> temp_pair;
        for (int i = 0; i < POPULATION_SIZE * 0.5; i++) { //pick random pairs from population best solutions and some random solutions
            first_rand = random_id(gen);
            do {
                second_rand = random_id(gen);
            } while(second_rand == first_rand);
            temp_pair.first = *best_of[first_rand];
            temp_pair.second = *best_of[second_rand];
            random_pairs.push_back(temp_pair);
        }
        for (auto i : random_pairs){ //crossover all randomly picked pairs to create new population
            population.push_back(crossover(i, instance));
        }
        population.insert(population.begin(), best_of.begin(), best_of.end());
///////////////////////////////////////////////////////////////// DO USTALENIA ZAKRES LICZBY MUTACJI
        uniform_int_distribution<> random_number(0, POPULATION_SIZE - 1);
        int mutation_number = random_number(gen);

        for(int i = 0; i < mutation_number; i++){
            int temp_id = random_number(gen);
            population[temp_id] = mutation(population[temp_id], instance);
        }
        sort(population.begin(), population.end(), by_finish_time);

        duration = (clock() - start ) / (double) CLOCKS_PER_SEC;
    }
    int best_time = population.front()->finish_time;
    cout << "Najlepszy czas dla populacji końcowej: " << best_time << endl;
    cout << "Najlepszy czas poprawiono o: " << init_time - best_time << endl;
}

// TODO: Zapis rozwiązania do pliku (w odpowiednim formacie) + testy + sprawozdanie

int main() {
    vector<inst> instancje;

    inst temp_inst;
    temp_inst = make_instance(N, K);
    instancje.push_back(temp_inst);

    genetic_algorithm(instancje[0]);
    return 0;
}