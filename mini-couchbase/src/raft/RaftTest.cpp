#include <iostream>
#include <string>
#include <thread>

#include "network/NodeNetwork.h"
#include "raft/ElectionManager.h"
#include "raft/RaftGroup.h"
#include "raft/RaftNetwork.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout
            << "Usage: raft_test <nodeId>\n";

        return 1;
    }

    int nodeId =
        std::stoi(argv[1]);

    if (nodeId < 0 || nodeId > 2) {
        std::cout
            << "Node ID must be 0, 1, or 2\n";

        return 1;
    }

    int port =
        9001 + nodeId;

    std::vector<int> peers;

    for (int id = 0; id < 3; ++id) {
        if (id != nodeId) {
            peers.push_back(id);
        }
    }

    RaftGroup group(
        nodeId,
        0,
        peers
    );

    NodeNetwork network(
        "127.0.0.1",
        port
    );

    RaftNetwork raftNetwork(
        network,
        nodeId,
        group
    );

    std::thread serverThread(
        [&network]() {
            network.start();
        }
    );

    serverThread.detach();

    std::this_thread::sleep_for(
        std::chrono::seconds(1)
    );

    std::cout
        << "Node "
        << nodeId
        << " started on port "
        << port
        << std::endl;

    ElectionManager electionManager(
        group,
        raftNetwork
    );

    std::cout
        << "Press E to start election"
        << std::endl;

    std::string command;

    while (std::getline(
        std::cin,
        command
    )) {
        if (command == "E") {
            std::cout
                << "Node "
                << nodeId
                << " starting election..."
                << std::endl;

            electionManager.startElection();

            std::cout
                << "Node "
                << nodeId
                << " became candidate"
                << std::endl;

            bool leader =
                electionManager.requestVotes();

            if (leader) {
                std::cout
                    << "Node "
                    << nodeId
                    << " became LEADER"
                    << std::endl;
            }
            else {
                std::cout
                    << "Node "
                    << nodeId
                    << " did not become leader"
                    << std::endl;
            }

            std::cout
                << "Term: "
                << group.getCurrentTerm()
                << std::endl;

            std::cout
                << "Votes: "
                << electionManager.getVotesReceived()
                << "/"
                << electionManager.getMajority()
                << std::endl;
        }
        else if (command == "S") {
            std::cout
                << "Node: "
                << group.getNodeId()
                << std::endl;

            std::cout
                << "Term: "
                << group.getCurrentTerm()
                << std::endl;

            std::cout
                << "Role: ";

            switch (group.getRole()) {
                case RaftRole::FOLLOWER:
                    std::cout << "FOLLOWER";
                    break;

                case RaftRole::CANDIDATE:
                    std::cout << "CANDIDATE";
                    break;

                case RaftRole::LEADER:
                    std::cout << "LEADER";
                    break;
            }

            std::cout << std::endl;

            std::cout
                << "Votes: "
                << electionManager.getVotesReceived()
                << std::endl;
        }
        else if (command == "EXIT") {
            break;
        }
        else {
            std::cout
                << "Commands: E, S, EXIT"
                << std::endl;
        }
    }

    return 0;
}