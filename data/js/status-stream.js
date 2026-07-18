export function connectStatusStream(handlers = {}) {
  const source = new EventSource("/api/events");
  const connectionEl = document.getElementById("connection");

  source.addEventListener("operation", (event) => {
    const operation = JSON.parse(event.data);
    handlers.onOperation?.(operation);
  });

  source.addEventListener("status", (event) => {
    const status = JSON.parse(event.data);
    handlers.onStatus?.(status);
  });

  source.addEventListener("fault", (event) => {
    const fault = JSON.parse(event.data);
    handlers.onFault?.(fault);
  });

  source.onopen = () => {
    if (connectionEl) {
      connectionEl.textContent = "Connected";
    }
    handlers.onConnection?.("connected");
  };

  source.onerror = () => {
    if (connectionEl) {
      connectionEl.textContent = "Reconnecting…";
    }
    handlers.onConnection?.("reconnecting");
  };

  return source;
}
