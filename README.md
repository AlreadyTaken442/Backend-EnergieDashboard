# Backend-EnergieDashboard
API-Backend | saves MQTT Data and sends when requestet

## Aktueller API-Startpunkt

### Verbindungs-/Health-Check

- `GET /`
- `GET /health`

### Auth

- `POST /auth/register` mit JSON `{ "name": "...", "email": "...", "password": "..." }`
- `POST /auth/login` mit JSON `{ "email": "...", "password": "..." }`
- `PUT /users` mit JSON `{ "benutzer_id": 1, "name": "...", "email": "...", "password": "...", "aktiv": true }`
- `DELETE /users` mit JSON `{ "benutzer_id": 1 }`

### Tabellen-Anbindungen (GET)

- `GET /users`
- `GET /notifications`
- `GET /user-roles`
- `GET /permissions`
- `GET /buildings`
- `GET /devices`
- `GET /device-types`
- `GET /usage-statistics`
- `GET /rooms`
- `GET /roles`
- `GET /role-permissions`
- `GET /sensor-data`

> Hinweis: Das aktuelle Passwort-Hashing/Tokening ist ein Startpunkt für die Entwicklung.
> Als nächster Schritt sollten Argon2/bcrypt und echte JWTs integriert werden.
