import type { IncomingMessage, Server, ServerResponse } from "node:http";

/** A handler for /omarchy/events, /omarchy/state, /omarchy/icons and /omarchy/icon/*; false for any other URL. */
export function omarchyHandler(options?: { env?: NodeJS.ProcessEnv; bin?: string }): ((req: IncomingMessage, res: ServerResponse) => boolean) & { stop(): void };

/** Serve `root` and the /omarchy/* routes on 127.0.0.1. */
export function serve(options?: { root?: string; port?: number; host?: string; env?: NodeJS.ProcessEnv }): Promise<{ server: Server; port: number; url: string; stop(): void }>;
