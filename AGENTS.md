# Backend development conventions

## Database models

- Every new database table must have a corresponding C++ `*Model` class in
  this repository.
- A model must cover every persisted column, using types compatible with the
  MySQL schema.
- DAO classes map database rows to models and return models or collections of
  models. DAOs must not assemble HTTP JSON responses.
- Service classes validate business input and serialize models into API JSON.
- New source or header files must also be registered in `ChatServer.vcxproj`
  and `ChatServer.vcxproj.filters` so Visual Studio builds include them.
