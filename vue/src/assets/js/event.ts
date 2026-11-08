interface IEventFunction {
  (arg: any): any;
}

interface TypeEvent {
  [key: string]: IEventFunction[];
}

class Event {
  events: TypeEvent;
  constructor() {
    this.events = {};
  }

  on(name: string, fn: IEventFunction) {
    this.events[name] = this.events[name] || [];
    this.events[name].push(fn);
  }

  off(name: string, fn: IEventFunction) {
    if (this.events[name]) {
      for (let i = 0; i < this.events[name].length; i++) {
        if (this.events[name][i] === fn) {
          this.events[name].splice(i, 1);
          break;
        }
      }
    }
  }

  emit(name: string, data?: unknown) {
    if (this.events[name]) {
      this.events[name].forEach((fn: IEventFunction) => {
        fn(data);
      });
    }
  }
}

export default Event;
