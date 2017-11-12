#pragma once
#include <utility>
#include <list>

namespace Resecs {

	/* Signal class for implementing event. */
	template <typename... TFuncArgs>
	class Signal {
	public:
		using Callback = std::function<void(TFuncArgs...)>;
		Signal() : survivePtr(make_shared<int>(0)) {
		}
		/* Connection class.
		Disconnect() will be called automatically once it's out of scope.
		*/
		class SignalConnection {
		private:
			friend class Signal;
			/* We only allow class Signal to create a connection. */
			SignalConnection(Signal& signal, int id, std::shared_ptr<int> survivePtr) noexcept:id(id), signal(signal), signalSurvivePtr(survivePtr){}
			int id;
			Signal& signal;
			bool disconnected = false;
			std::weak_ptr<int> signalSurvivePtr;
		public:
			/* A copy constructor of "connection" is really confusing. just delete it. */
			SignalConnection(const SignalConnection& copy) = delete;
			/* without a copy constructor, we can't return SignalConnection, unless we provide a move constructor. */
			SignalConnection(SignalConnection&& toMove) noexcept : id(toMove.id), signal(toMove.signal), disconnected(toMove.disconnected), signalSurvivePtr(toMove.signalSurvivePtr) {}
			~SignalConnection() {
				Disconnect();
			}
			void Disconnect() {
				if (disconnected || signalSurvivePtr.expired())
					return;
				disconnected = true;
				signal.Disconnect(*this);
			}
		};

		/* <b>Register to the Signal.</b>
		Returns a connection object.
		the connection object will automatically disconnect once it's out of scope.*/
		SignalConnection Connect(Callback callback) {
			callbacks.push_back(std::pair<int, Callback>(idRoller++, callback));
			return SignalConnection(*this, idRoller - 1, survivePtr);
		}

		void Invoke(TFuncArgs... args) {
			for (auto& con : callbacks) {
				(con.second)(args...);
			}
		}

		void operator()(TFuncArgs... args) {
			Invoke(args...);
		}

	private:
		/* ID Counter.
		We look for the connection's corresponding callback using index, since the operator== of std::function doesn't work as imagine.
		*/
		int idRoller = 0;
		std::list<std::pair<int, Callback>> callbacks;
		std::shared_ptr<int> survivePtr;

		/* Only SignalConnection can call this method.
		*/
		void Disconnect(SignalConnection& t) {
			callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(), [&](auto& pCallback) {
				return pCallback.first == t.id;
			}), callbacks.end());
		}
	};
}