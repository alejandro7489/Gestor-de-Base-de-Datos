# Guía para asistentes de IA — Spotify Database Manager

> **LECTURA OBLIGATORIA antes de editar este repositorio.** Ya hay un scaffold C++17. No crear otro ni reestructurar el proyecto.

1. Leer **completos** [`docs/PLAN_DE_TRABAJO_IA.md`](docs/PLAN_DE_TRABAJO_IA.md) (alcance, objetivos, responsables y prompts por integrante) y [`docs/contratos.md`](docs/contratos.md) (formato binario, interfaces, reglas de memoria).
2. Revisar también [`docs/arquitectura.md`](docs/arquitectura.md), [`README.md`](README.md) y el código real antes de proponer cambios.
3. Confirmar el **rol**: Fernando modifica Storage; Leonardo modifica Buffer; Esteban modifica Hash, Gestor y pruebas. **Nunca tocar archivos ajenos sin acuerdo.**
4. La entrega del **23 de noviembre de 2026** se centra en CSV -> páginas binarias -> Buffer Pool con **hits y misses reales**. **LRU no se implementa todavía**; conservar sus stubs.
5. No cambiar contratos públicos ni serialización sin aprobación; no regenerar el scaffold, no inventar datos ni estadísticas, no borrar/subir CSV o binarios, no hacer commits/push/merge sin autorización.
6. Código claro de **cuarto semestre**, C++17, pruebas reproducibles y explicación final de archivos modificados, pruebas ejecutadas y pendientes.

**Fuente de verdad:** si existe un desacuerdo entre el código actual y los documentos, informar al responsable del módulo antes de cambiar la interfaz.
