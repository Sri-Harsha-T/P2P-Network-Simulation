client-phase: client-phase1.cpp client-phase2.cpp client-phase3.cpp client-phase4.cpp client-phase5.cpp 
	g++ client-phase1.cpp -o client-phase1 -lcrypto -pthread
	g++ client-phase2.cpp -o client-phase2 -lcrypto -pthread
	g++ client-phase3.cpp -o client-phase3 -lcrypto -pthread
	g++ client-phase4.cpp -o client-phase4 -lcrypto -pthread
	g++ client-phase5.cpp -o client-phase5 -lcrypto -pthread

