#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <semaphore>
#include <atomic>
#include <chrono>
#include <random>
#include <cstdlib>
#include <ctime>

using namespace std;
// Global variables for synchronization
constexpr int NUM_JOGADORES = 4;
std::counting_semaphore<NUM_JOGADORES> cadeira_sem(NUM_JOGADORES - 1); // Inicia com n-1 cadeiras, capacidade máxima n
std::condition_variable music_cv;
std::mutex music_mutex;
std::mutex jogador_mutex;
std::condition_variable jogador_cv;
std::atomic<bool> musica_parada{false};
std::atomic<bool> jogo_ativo{true};
int i = NUM_JOGADORES-1;

class JogoDasCadeiras;
class Jogador;
std::vector<Jogador> jogadores_objs;

/*
 * Uso básico de um counting_semaphore em C++:
 * 
 * O `std::counting_semaphore` é um mecanismo de sincronização que permite controlar o acesso a um recurso compartilhado 
 * com um número máximo de acessos simultâneos. Neste projeto, ele é usado para gerenciar o número de cadeiras disponíveis.
 * Inicializamos o semáforo com `n - 1` para representar as cadeiras disponíveis no início do jogo. 
 * Cada jogador que tenta se sentar precisa fazer um `acquire()`, e o semáforo permite que até `n - 1` jogadores 
 * ocupem as cadeiras. Quando todos os assentos estão ocupados, jogadores adicionais ficam bloqueados até que 
 * o coordenador libere o semáforo com `release()`, sinalizando a eliminação dos jogadores.
 * O método `release()` também pode ser usado para liberar múltiplas permissões de uma só vez, por exemplo: `cadeira_sem.release(3);`,
 * o que permite destravar várias threads de uma só vez, como é feito na função `liberar_threads_eliminadas()`.
 *
 * Métodos da classe `std::counting_semaphore`:
 * 
 * 1. `acquire()`: Decrementa o contador do semáforo. Bloqueia a thread se o valor for zero.
 *    - Exemplo de uso: `cadeira_sem.acquire();` // Jogador tenta ocupar uma cadeira.
 * 
 * 2. `release(int n = 1)`: Incrementa o contador do semáforo em `n`. Pode liberar múltiplas permissões.
 *    - Exemplo de uso: `cadeira_sem.release(2);` // Libera 2 permissões simultaneamente.
 */

// Classes
class JogoDasCadeiras {
public:
    JogoDasCadeiras(int num_jogadores)
        : num_jogadores(num_jogadores), cadeiras(num_jogadores-1) {}

    void iniciar_rodada() {
        // TODO: Inicia uma nova rodada, removendo uma cadeira e ressincronizando o semáforo
        musica_parada = false;
        this->cadeiras--;
        i=this->cadeiras;
        cadeira_sem.release(cadeiras);
        if(jogo_ativo){
            this->exibir_estado();
            cout << "A musica esta tocando... " << endl << endl;
        }
    }

    void parar_musica() {
        // TODO: Simula o momento em que a música para e notifica os jogadores via variável de condição
        musica_parada = true;
        music_cv.notify_all();
        int cadeiras_livres=this->cadeiras;
        if(cadeiras_livres== 1){
            jogo_ativo = 0;
        }
        if(this->cadeiras>1) cout << "A musica parou! Os jogadores estão tentando ocupar as cadeiras..." << endl;
        else cout << "A musica parou! Os jogadores estão tentando ocupar a cadeira..." << endl;
        cout << "--------------------------------------------------------------" << endl << endl;
    }

    void eliminar_jogador(int jogador_id) {
        // TODO: Elimina um jogador que não conseguiu uma cadeira
        this->num_jogadores--;

    }

    void exibir_estado() {
        // TODO: Exibe o estado atual das cadeiras e dos jogadores
        if(this->cadeiras>1)
            cout << "Há " << num_jogadores << " jogadores e "
             << cadeiras << " cadeiras" << endl << endl;
        else 
            cout << "Há " << num_jogadores << " jogadores e "
             << cadeiras << " cadeira" << endl << endl;
    }
    int get_num_cadeiras(){
        return this->cadeiras;
    }

private:
    int num_jogadores;
    int cadeiras;
};

class Jogador {
public:
    bool ativo;
    Jogador(int id, JogoDasCadeiras& jogo)
        : id(id), jogo(jogo), ativo(true) {}

    void tentar_ocupar_cadeira() {
        // TODO: Tenta ocupar uma cadeira utilizando o semáforo contador quando a música para (aguarda pela variável de condição)
        unique_lock<mutex> lock(music_mutex);
            music_cv.wait(lock);
        if(i>0){
            cadeira_sem.acquire();
            i--;
            return;
        }
        this->ativo = false;
        this->jogo.eliminar_jogador(this->id);
    }

    bool verificar_eliminacao() {
        // TODO: Verifica se foi eliminado após ser destravado do semáforo   
        return this->ativo;    
    }

    void joga() {
        // TODO: Aguarda a música parar usando a variavel de condicao
        // TODO: Tenta ocupar uma cadeira
        // TODO: Verifica se foi eliminado
        while(this->ativo&&jogo_ativo){
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
                tentar_ocupar_cadeira();
                std::this_thread::sleep_for(std::chrono::milliseconds(2100));
                if(!this->ativo) cout << "Jogador P" << this->id << " foi eliminado" << endl << endl;
                unique_lock<mutex> lock(jogador_mutex);
                jogador_cv.wait(lock);
        }
    }

    int get_id(){
        return this->id;
    }

private:
    int id;
    JogoDasCadeiras& jogo;
};

class Coordenador {
public:
    Coordenador(JogoDasCadeiras& jogo)
        : jogo(jogo) {}

    void iniciar_jogo() {
        // TODO: Começa o jogo, dorme por um período aleatório, e então para a música, sinalizando os jogadores
        cout << "--------------------------------------------------------------" << endl;
        cout << "               Bem-vindo ao jogo das cadeiras!" << endl;
        cout << "--------------------------------------------------------------" << endl << endl;
        this->jogo.exibir_estado();
        cout << "A musica esta tocando... " << endl << endl;
        int tempo = 2000 + rand() % 10000;
        while(jogo_ativo){
            std::this_thread::sleep_for(std::chrono::milliseconds(tempo));
            this->jogo.parar_musica();
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            int count=1;
            for(auto it : jogadores_objs){
                if(it.ativo){
                    cout << "[Cadeira " << count << "]: ocupada por P" 
                        << it.get_id()<< endl;
                    count++;
                }
            }
            cout << "--------------------------------------------------------------"  << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            this->jogo.iniciar_rodada();
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            jogador_cv.notify_all();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }   

    }

    void liberar_threads_eliminadas() {
        // Libera múltiplas permissões no semáforo para destravar todas as threads que não conseguiram se sentar
        cadeira_sem.release(NUM_JOGADORES - 1); // Libera o número de permissões igual ao número de jogadores que ficaram esperando

    }

private:
    JogoDasCadeiras& jogo;
};

// Main function
int main() {
    unsigned seed = time(0);
    srand(seed);
    JogoDasCadeiras jogo(NUM_JOGADORES);
    Coordenador coordenador(jogo);
    std::vector<std::thread> jogadores;
    // Criação das threads dos jogadores
    for (int i = 1; i <= NUM_JOGADORES; ++i) {
        jogadores_objs.emplace_back(i, jogo);
    }

    for (int i = 0; i < NUM_JOGADORES; ++i) {
        jogadores.emplace_back(&Jogador::joga, &jogadores_objs[i]);
    }

    // Thread do coordenador
    std::thread coordenador_thread(&Coordenador::iniciar_jogo, &coordenador);

    // Esperar pelas threads dos jogadores
    for (auto& t : jogadores) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Esperar pela thread do coordenador
    if (coordenador_thread.joinable()) {
        coordenador_thread.join();
    }
    cout << "--------------------------------------------------------------" << endl;
    for(auto i : jogadores_objs){
        if(i.ativo) cout << "Jogador P" << i.get_id() << " é o vencedor!" << endl;
    }
    cout << "--------------------------------------------------------------" << endl << endl;
    std::cout << "Jogo das Cadeiras finalizado." << std::endl << endl;
    return 0;
}

